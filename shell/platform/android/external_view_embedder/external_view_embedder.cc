// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/android/external_view_embedder/external_view_embedder.h"
#include "flutter/shell/platform/android/platform_view_android_jni.h"
#include "flutter/fml/trace_event.h"

namespace flutter {

AndroidExternalViewEmbedder::AndroidExternalViewEmbedder(fml::jni::JavaObjectWeakGlobalRef java_object) : java_object_(java_object) {
  
}

// |ExternalViewEmbedder|
void AndroidExternalViewEmbedder::PrerollCompositeEmbeddedView(
    int view_id,
    std::unique_ptr<EmbeddedViewParams> params) {
  // TODO(egarciad): Implement hybrid composition.
  // https://github.com/flutter/flutter/issues/55270
  TRACE_EVENT0("flutter",
               "AndroidExternalViewEmbedder::PrerollCompositeEmbeddedView");
  picture_recorders_[view_id] = std::make_unique<SkPictureRecorder>();
  picture_recorders_[view_id]->beginRecording(SkRect::Make(frame_size_));

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

// |ExternalViewEmbedder|
bool AndroidExternalViewEmbedder::SubmitFrame(GrContext* context,
                                              SkCanvas* background_canvas) {
  // TODO(egarciad): Implement hybrid composition.
  // https://github.com/flutter/flutter/issues/55270
  TRACE_EVENT0("flutter", "AndroidExternalViewEmbedder::SubmitFrame");
  for (size_t i = 0; i < composition_order_.size(); i++) {
    int64_t view_id = composition_order_[i];
    background_canvas->drawPicture(
        picture_recorders_[view_id]->finishRecordingAsPicture());
  }


  return true;
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
  frame_size_ = SkISize::MakeEmpty();
}

// |ExternalViewEmbedder|
void AndroidExternalViewEmbedder::CancelFrame() {
  ClearFrame();
}

// |ExternalViewEmbedder|
void AndroidExternalViewEmbedder::FinishFrame() {
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

  ClearFrame();
}

// |ExternalViewEmbedder|
void AndroidExternalViewEmbedder::EndFrame(
    fml::RefPtr<fml::RasterThreadMerger> raster_thread_merger) {


}

}  // namespace flutter
