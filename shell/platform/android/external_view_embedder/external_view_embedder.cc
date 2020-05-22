// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/android/external_view_embedder/external_view_embedder.h"

#include "flutter/shell/platform/android/platform_view_android_jni.h"
#include "flutter/fml/trace_event.h"

namespace flutter {

AndroidExternalViewEmbedder::AndroidExternalViewEmbedder(fml::jni::JavaObjectWeakGlobalRef java_object) 
  : layer_pool_(std::make_unique<platform_view::Pool>()),
    java_object_(java_object) {}

// |ExternalViewEmbedder|
void AndroidExternalViewEmbedder::PrerollCompositeEmbeddedView(
    int view_id,
    std::unique_ptr<EmbeddedViewParams> params) {
  // TODO(egarciad): Implement hybrid composition.
  // https://github.com/flutter/flutter/issues/55270
  TRACE_EVENT0("flutter",
               "AndroidExternalViewEmbedder::PrerollCompositeEmbeddedView");
  auto rtree_factory = RTreeFactory();
  platform_view_rtrees_[view_id] = rtree_factory.getInstance();

  picture_recorders_[view_id] = std::make_unique<SkPictureRecorder>();
  picture_recorders_[view_id]->beginRecording(SkRect::Make(frame_size_), &rtree_factory);

  composition_order_.push_back(view_id);
  if (current_composition_params_.count(view_id) == 1 &&
      current_composition_params_[view_id] == *params.get()) {
    // Do nothing if the params didn't change.
    return;
  }
 
  current_composition_params_[view_id] = EmbeddedViewParams(*params.get());
}

// |ExternalViewEmbedder|
SkCanvas* AndroidExternalViewEmbedder::CompositeEmbeddedView(int view_id) {
  return picture_recorders_[view_id]->getRecordingCanvas();
}

// |ExternalViewEmbedder|
std::vector<SkCanvas*> AndroidExternalViewEmbedder::GetCurrentCanvases() {
  // TODO(egarciad): Implement hybrid composition.
  // https://github.com/flutter/flutter/issues/55270
  std::vector<SkCanvas*> canvases;
  for (size_t i = 0; i < composition_order_.size(); i++) {
    int64_t view_id = composition_order_[i];
    canvases.push_back(picture_recorders_[view_id]->getRecordingCanvas());
  }
  return canvases;
}

SkRect AndroidExternalViewEmbedder::GetPlatformViewRect(int view_id) {
  EmbeddedViewParams* params = &current_composition_params_[view_id];
  FML_CHECK(params != nullptr);

  return SkRect::MakeXYWH(params->offsetPixels.x(),     //
                          params->offsetPixels.y(),     //
                          params->sizePoints.width(),   //
                          params->sizePoints.height()   //
  );
}

// |ExternalViewEmbedder|
bool AndroidExternalViewEmbedder::SubmitFrame(GrContext* context,
                                              SkCanvas* background_canvas) {
  // TODO(egarciad): Implement hybrid composition.
  // https://github.com/flutter/flutter/issues/55270
  TRACE_EVENT0("flutter", "AndroidExternalViewEmbedder::SubmitFrame");

  SkAutoCanvasRestore save(background_canvas, /*doSave=*/true);

  platform_view::CompositeOrderMap overlay_layers;

  auto did_submit = true;
  auto num_platform_views = composition_order_.size();

  for (size_t i = 0; i < num_platform_views; i++) {
    int64_t platform_view_id = composition_order_[i];
    sk_sp<RTree> rtree = platform_view_rtrees_[platform_view_id];
    sk_sp<SkPicture> picture = picture_recorders_[platform_view_id]->finishRecordingAsPicture();

    for (size_t j = i + 1; j > 0; j--) {
      int64_t current_platform_view_id = composition_order_[j - 1];
      SkRect platform_view_rect = GetPlatformViewRect(current_platform_view_id);

      std::list<SkRect> intersection_rects =
          rtree->searchNonOverlappingDrawnRects(platform_view_rect);
      auto allocation_size = intersection_rects.size();
      auto overlay_id = overlay_layers[current_platform_view_id].size();

      if (allocation_size > kMaxLayerAllocations) {
        SkRect joined_rect;
        for (const SkRect& rect : intersection_rects) {
          joined_rect.join(rect);
        }
        // Replace the rects in the intersection rects list for a single rect that is
        // the union of all the rects in the list.
        intersection_rects.clear();
        intersection_rects.push_back(joined_rect);
      }
      for (SkRect& joined_rect : intersection_rects) {
        // Get the intersection rect between the current rect
        // and the platform view rect.
       // joined_rect.intersect(platform_view_rect);
        // Subpixels in the platform may not align with the canvas subpixels.
        // To workaround it, round the floating point bounds and make the rect slighly larger.
        // For example, {0.3, 0.5, 3.1, 4.7} becomes {0, 0, 4, 5}.
        // joined_rect.setLTRB(std::floor(joined_rect.left()), std::floor(joined_rect.top()),
        //                     std::ceil(joined_rect.right()), std::ceil(joined_rect.bottom()));
        // Clip the background canvas, so it doesn't contain any of the pixels drawn
        // on the overlay layer.
        background_canvas->clipRect(joined_rect, SkClipOp::kDifference);
        // Get a new host layer.
        std::shared_ptr<platform_view::OverlayLayer> layer = GetLayer(context,                   //
                                                                      picture,                   //
                                                                      joined_rect,               //
                                                                      current_platform_view_id,  //
                                                                      overlay_id                 //
        );
        did_submit &= layer->did_submit_last_frame;
        overlay_layers[current_platform_view_id].push_back(layer);
        overlay_id++;
      }
    }
    background_canvas->drawPicture(picture);
  }

  int64_t view_id = 0;
  EmbeddedViewParams* params = &current_composition_params_[view_id];

  JNIEnv* env = fml::jni::AttachCurrentThread();
  fml::jni::ScopedJavaLocalRef<jobject> view = java_object_.get(env);

  FlutterViewOnPositionPlatformView(fml::jni::AttachCurrentThread(), view.obj(), view_id,
    params->offsetPixels.x(),
    params->offsetPixels.y(),
    params->sizePoints.width(),
    params->sizePoints.height()
  );

  FML_LOG(ERROR) << "compositing embedded view: " << view_id << ", at: " << params->offsetPixels.x()
    << ", " << params->offsetPixels.y() << ", "
    << " (" << params->sizePoints.width() << "x" << params->sizePoints.height() << ")";

  composition_order_.clear();
  layer_pool_->RecycleLayers();
  return did_submit;
}

std::shared_ptr<platform_view::OverlayLayer> AndroidExternalViewEmbedder::GetLayer(
    GrContext* gr_context,
    sk_sp<SkPicture> picture,
    SkRect rect,
    int64_t view_id,
    int64_t overlay_id) {
  std::shared_ptr<platform_view::OverlayLayer> layer = layer_pool_->GetLayer(gr_context, java_object_);
  std::unique_ptr<SurfaceFrame> frame = layer->surface->AcquireFrame(frame_size_, true);

  JNIEnv* env = fml::jni::AttachCurrentThread();
  fml::jni::ScopedJavaLocalRef<jobject> view = java_object_.get(env);


  SkCanvas* overlay_canvas = frame->SkiaCanvas();
  overlay_canvas->clear(SK_ColorTRANSPARENT);
  // Offset the picture since its absolute position on the scene is determined
  // by the position of the overlay view.
  overlay_canvas->translate(-rect.x(), -rect.y());
  overlay_canvas->drawPicture(picture);
  layer->did_submit_last_frame = frame->Submit();

  FlutterViewPositionOverlayLayer(fml::jni::AttachCurrentThread(), view.obj(), layer->id, rect.x(), rect.y(), rect.width(), rect.height());

  return layer;
}

// |ExternalViewEmbedder|
PostPrerollResult AndroidExternalViewEmbedder::PostPrerollAction(
    fml::RefPtr<fml::RasterThreadMerger> raster_thread_merger) {
  return PostPrerollResult::kSuccess;
}

// |ExternalViewEmbedder|
SkCanvas* AndroidExternalViewEmbedder::GetRootCanvas() {
  // On Android, the root surface is created from the on-screen render target.
  return nullptr;
}

// |ExternalViewEmbedder|
void AndroidExternalViewEmbedder::BeginFrame(SkISize frame_size,
                                             GrContext* context,
                                             double device_pixel_ratio) {
  frame_size_ = frame_size;
}

void AndroidExternalViewEmbedder::ClearFrame() {
  composition_order_.clear();
  picture_recorders_.clear();
  current_composition_params_.clear();
  platform_view_rtrees_.clear();
  frame_size_ = SkISize::MakeEmpty();
  layer_pool_->RecycleLayers();
}

// |ExternalViewEmbedder|
void AndroidExternalViewEmbedder::CancelFrame() {
  ClearFrame();
}

// |ExternalViewEmbedder|
void AndroidExternalViewEmbedder::FinishFrame() {
  // int64_t view_id = 0;

  // EmbeddedViewParams* params = &current_composition_params_[view_id];


  // JNIEnv* env = fml::jni::AttachCurrentThread();
  // fml::jni::ScopedJavaLocalRef<jobject> view = java_object_.get(env);

  // FlutterViewOnPositionPlatformView(fml::jni::AttachCurrentThread(), view.obj(), view_id,
  //   params->offsetPixels.x(),
  //   params->offsetPixels.y(),
  //   params->sizePoints.width(),
  //   params->sizePoints.height()
  // );

  // FML_LOG(ERROR) << "compositing embedded view: " << view_id << ", at: " << params->offsetPixels.x()
  //   << ", " << params->offsetPixels.y() << ", "
  //   << " (" << params->sizePoints.width() << "x" << params->sizePoints.height() << ")";

  ClearFrame();
}

// |ExternalViewEmbedder|
void AndroidExternalViewEmbedder::EndFrame(
    fml::RefPtr<fml::RasterThreadMerger> raster_thread_merger) {


}

}  // namespace flutter
