// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_PLATFORM_VIEW_POOL_H_
#define FLUTTER_PLATFORM_VIEW_POOL_H_

#include "flutter/shell/platform/android/android_surface.h"

namespace flutter {

namespace platform_view {

struct OverlayLayer {
  OverlayLayer(long id,
               std::unique_ptr<AndroidSurface> android_surface,
               std::unique_ptr<Surface> surface);

  ~OverlayLayer();

  long id;

  std::unique_ptr<AndroidSurface> android_surface;

  std::unique_ptr<Surface> surface;

  // Whether a frame for this layer was submitted.
  bool did_submit_last_frame;

  // The GrContext that is currently used by the overlay surfaces.
  // We track this to know when the GrContext for the Flutter app has changed
  // so we can update the overlay with the new context.
  GrContext* gr_context;
};

using CompositeOrderMap = std::map<int64_t, std::vector<std::shared_ptr<OverlayLayer>>>;

// This class isn't thread safe.
class Pool {
 public:
  Pool() = default;
  ~Pool() = default;

  // Gets a layer from the pool if available, or allocates a new one.
  // Finally, it marks the layer as used. That is, it increments `available_layer_index_`.
  std::shared_ptr<OverlayLayer> GetLayer(GrContext* gr_context, fml::jni::JavaObjectWeakGlobalRef java_object);

  // Gets the layers in the pool that aren't currently used.
  // This method doesn't mark the layers as unused.
  std::vector<std::shared_ptr<OverlayLayer>> GetUnusedLayers();

  // Marks the layers in the pool as available for reuse.
  void RecycleLayers();

 private:
  // The index of the entry in the layers_ vector that determines the beginning of the unused
  // layers. For example, consider the following vector:
  //  _____
  //  | 0 |
  /// |---|
  /// | 1 | <-- available_layer_index_
  /// |---|
  /// | 2 |
  /// |---|
  ///
  /// This indicates that entries starting from 1 can be reused meanwhile the entry at position 0
  /// cannot be reused.
  size_t available_layer_index_ = 0;
  std::vector<std::shared_ptr<OverlayLayer>> layers_;
};

}  // namespace platform_view

}  // namespace flutter

#endif  // FLUTTER_PLATFORM_VIEW_POOL_H_
