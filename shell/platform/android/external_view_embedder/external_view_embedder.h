// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_ANDROID_EXTERNAL_VIEW_EMBEDDER_H_
#define FLUTTER_SHELL_PLATFORM_ANDROID_EXTERNAL_VIEW_EMBEDDER_H_

#include "flutter/flow/rtree.h"
#include "flutter/fml/platform/android/jni_util.h"
#include "flutter/fml/platform/android/jni_weak_ref.h"
#include "flutter/flow/embedded_views.h"
#include "flutter/shell/platform/android/external_view_embedder/pool.h"
#include "third_party/skia/include/core/SkPictureRecorder.h"

namespace flutter {

class AndroidExternalViewEmbedder : public ExternalViewEmbedder {
 public:
  AndroidExternalViewEmbedder(fml::jni::JavaObjectWeakGlobalRef java_object);

  // |ExternalViewEmbedder|
  void PrerollCompositeEmbeddedView(
      int view_id,
      std::unique_ptr<flutter::EmbeddedViewParams> params) override;

  // |ExternalViewEmbedder|
  SkCanvas* CompositeEmbeddedView(int view_id) override;

  // |ExternalViewEmbedder|
  std::vector<SkCanvas*> GetCurrentCanvases() override;

  // |ExternalViewEmbedder|
  bool SubmitFrame(GrContext* context, SkCanvas* background_canvas) override;

  // |ExternalViewEmbedder|
  PostPrerollResult PostPrerollAction(
      fml::RefPtr<fml::RasterThreadMerger> raster_thread_merger) override;

  // |ExternalViewEmbedder|
  SkCanvas* GetRootCanvas() override;

  // |ExternalViewEmbedder|
  void BeginFrame(SkISize frame_size,
                  GrContext* context,
                  double device_pixel_ratio) override;

  // |ExternalViewEmbedder|
  void CancelFrame() override;

  // |ExternalViewEmbedder|
  void FinishFrame() override;

  // |ExternalViewEmbedder|
  void EndFrame(
      fml::RefPtr<fml::RasterThreadMerger> raster_thread_merger) override;

 private:
  static const size_t kMaxLayerAllocations = 2;
  
  // The size of the background canvas.
  SkISize frame_size_;

  // The order of composition. Each entry contains a unique id for the platform
  // view.
  std::vector<int64_t> composition_order_;

  // The platform view's picture recorder keyed off the platform view id, which
  // contains any subsequent operation until the next platform view or the end
  // of the last leaf node in the layer tree.
  std::map<int64_t, std::unique_ptr<SkPictureRecorder>> picture_recorders_;

  std::map<int64_t, sk_sp<RTree>> platform_view_rtrees_;

  // The pool of reusable view layers. The pool allows to recycle layer in each frame.
  std::unique_ptr<platform_view::Pool> layer_pool_;

  std::map<int64_t, EmbeddedViewParams> current_composition_params_;

  fml::jni::JavaObjectWeakGlobalRef java_object_;

  /// Resets the state.
  void ClearFrame();

  std::shared_ptr<platform_view::OverlayLayer> GetLayer(GrContext* gr_context,
                                                        sk_sp<SkPicture> picture,
                                                        SkRect rect,
                                                        int64_t view_id,
                                                        int64_t overlay_id);

  SkRect GetPlatformViewRect(int view_id);
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_ANDROID_EXTERNAL_VIEW_EMBEDDER_H_
