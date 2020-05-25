// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/android/external_view_embedder/external_view_embedder.h"

#include "flutter/shell/platform/android/platform_view_android_jni.h"
#include "flutter/fml/trace_event.h"

namespace flutter {

// #define GPU_GL_RGBA8 0x8058
// #define GPU_GL_RGBA4 0x8056
// #define GPU_GL_RGB565 0x8D62

// static SkColorType FirstSupportedColorType(GrContext* context,
//                                            GrGLenum* format) {
// #define RETURN_IF_RENDERABLE(x, y)                 \
//   if (context->colorTypeSupportedAsSurface((x))) { \
//     *format = (y);                                 \
//     return (x);                                    \
//   }
//   RETURN_IF_RENDERABLE(kRGBA_8888_SkColorType, GPU_GL_RGBA8);
//   RETURN_IF_RENDERABLE(kARGB_4444_SkColorType, GPU_GL_RGBA4);
//   RETURN_IF_RENDERABLE(kRGB_565_SkColorType, GPU_GL_RGB565);
//   return kUnknown_SkColorType;
// }

// static sk_sp<SkSurface> WrapOnscreenSurface(GrContext* context,
//                                             const SkISize& size,
//                                             intptr_t fbo) {
//   GrGLenum format;
//   const SkColorType color_type = FirstSupportedColorType(context, &format);

//   GrGLFramebufferInfo framebuffer_info = {};
//   // framebuffer_info.fFBOID = static_cast<GrGLuint>(fbo);
//   framebuffer_info.fFormat = format;

//   GrBackendRenderTarget render_target(size.width(),     // width
//                                       size.height(),    // height
//                                       0,                // sample count
//                                       0,                // stencil bits (TODO)
//                                       framebuffer_info  // framebuffer info
//   );

//   sk_sp<SkColorSpace> colorspace = SkColorSpace::MakeSRGB();

//   SkSurfaceProps surface_props(
//       SkSurfaceProps::InitType::kLegacyFontHost_InitType);

//   return SkSurface::MakeFromBackendRenderTarget(
//       context,                                       // gr context
//       render_target,                                 // render target
//       GrSurfaceOrigin::kBottomLeft_GrSurfaceOrigin,  // origin
//       color_type,                                    // color type
//       colorspace,                                    // colorspace
//       &surface_props                                 // surface properties
//   );
// }

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

  return SkRect::MakeXYWH(params->offsetPixels.x(),                           //
                          params->offsetPixels.y(),                           //
                          params->sizePoints.width() * device_pixel_ratio_,   //
                          params->sizePoints.height() * device_pixel_ratio_   //
  );
}

// |ExternalViewEmbedder|
bool AndroidExternalViewEmbedder::SubmitFrame(GrContext* context,
                                              SkCanvas* background_canvas,
                                              std::function<void()> make_context) {
  // TODO(egarciad): Implement hybrid composition.
  // https://github.com/flutter/flutter/issues/55270
  TRACE_EVENT0("flutter", "AndroidExternalViewEmbedder::SubmitFrame");

  // Resolve all pending GPU operations before allocating a new surface.
  //background_canvas->flush();

  SkAutoCanvasRestore save(background_canvas, /*doSave=*/true);

  platform_view::CompositeOrderMap overlay_layers;

  JNIEnv* env = fml::jni::AttachCurrentThread();
  fml::jni::ScopedJavaLocalRef<jobject> view = java_object_.get(env);

  auto did_submit = true;
  auto num_platform_views = composition_order_.size();

  for (size_t i = 0; i < num_platform_views; i++) {
    int64_t platform_view_id = composition_order_[i];
    sk_sp<RTree> rtree = platform_view_rtrees_[platform_view_id];
    sk_sp<SkPicture> picture = picture_recorders_[platform_view_id]->finishRecordingAsPicture();

    // Position the platform view in the embedding.
    FlutterViewOnPositionPlatformView(
      fml::jni::AttachCurrentThread(),
      view.obj(),
      platform_view_id,
      current_composition_params_[platform_view_id].offsetPixels.x(),
      current_composition_params_[platform_view_id].offsetPixels.y(),
      current_composition_params_[platform_view_id].sizePoints.width(),
      current_composition_params_[platform_view_id].sizePoints.height()
    );

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
        joined_rect.setLTRB(std::floor(joined_rect.left()), std::floor(joined_rect.top()),
                            std::ceil(joined_rect.right()), std::ceil(joined_rect.bottom()));
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
        // make_context();
        did_submit &= layer->did_submit_last_frame;
        overlay_layers[current_platform_view_id].push_back(layer);
        overlay_id++;
      }
    }
    background_canvas->drawPicture(picture);
  }

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


  // sk_sp<SkSurface> surface = WrapOnscreenSurface(gr_context, frame_size_, 0);
  // SkCanvas* overlay_canvas = surface->getCanvas();


  // // SkCanvas* overlay_canvas = frame->SkiaCanvas();
  // overlay_canvas->clear(SK_ColorTRANSPARENT);
  // // Offset the picture since its absolute position on the scene is determined
  // // by the position of the overlay view.
  // overlay_canvas->flush();




  JNIEnv* env = fml::jni::AttachCurrentThread();
  fml::jni::ScopedJavaLocalRef<jobject> view = java_object_.get(env);




  // Position the overlay.
  FlutterViewPositionOverlayLayer(
    fml::jni::AttachCurrentThread(),    //
    view.obj(),                         //
    layer->id,                          //
    rect.x(),                           //
    rect.y(),                           //
    rect.width(),                       //
    rect.height()                       //
  );


  // SkCanvas* overlay_canvas = frame->getCanvas();


  SkCanvas* overlay_canvas = frame->SkiaCanvas();
  overlay_canvas->clear(SK_ColorTRANSPARENT);
  // Offset the picture since its absolute position on the scene is determined
  // by the position of the overlay view.
  overlay_canvas->translate(-rect.x(), -rect.y());
  overlay_canvas->drawPicture(picture);
  layer->did_submit_last_frame = frame->Submit();
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
  device_pixel_ratio_ = device_pixel_ratio;

  JNIEnv* env = fml::jni::AttachCurrentThread();
  fml::jni::ScopedJavaLocalRef<jobject> view = java_object_.get(env);
  FlutterViewBeginFrame(env, view.obj());
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
  JNIEnv* env = fml::jni::AttachCurrentThread();
  fml::jni::ScopedJavaLocalRef<jobject> view = java_object_.get(env);
  FlutterViewEndFrame(env, view.obj());
  ClearFrame();
}

// |ExternalViewEmbedder|
void AndroidExternalViewEmbedder::EndFrame(
    fml::RefPtr<fml::RasterThreadMerger> raster_thread_merger) {


}

}  // namespace flutter
