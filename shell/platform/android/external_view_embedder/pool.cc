// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/android/external_view_embedder/pool.h"

#include "flutter/shell/platform/android/android_surface.h"
#include "flutter/shell/platform/android/platform_view_android_jni.h"

namespace flutter {

namespace platform_view {

OverlayLayer::OverlayLayer(
  long id,
  std::unique_ptr<AndroidSurface> android_surface,
  std::unique_ptr<Surface> surface)
    : id(id),
      android_surface(std::move(android_surface)),
      surface(std::move(surface)){};

OverlayLayer::~OverlayLayer() = default;

std::shared_ptr<OverlayLayer> Pool::GetLayer(GrContext* gr_context, std::shared_ptr<AndroidContextGL> android_context, fml::jni::JavaObjectWeakGlobalRef java_object) {
  if (available_layer_index_ >= layers_.size()) {
    std::unique_ptr<AndroidSurface> android_surface = AndroidSurface::Create(false, android_context, java_object);

    JNIEnv* env = fml::jni::AttachCurrentThread();
    fml::jni::ScopedJavaLocalRef<jobject> view = java_object.get(env);

    std::unique_ptr<AndroidFlutterOverlayLayer> overlay_layer = FlutterViewCreateOverlayLayer(fml::jni::AttachCurrentThread(), view.obj());

    FML_DCHECK(android_surface->SetNativeWindow(overlay_layer->GetWindow()));

    std::unique_ptr<Surface> surface = android_surface->CreateGPUSurface(gr_context);
    std::shared_ptr<OverlayLayer> layer = std::make_shared<OverlayLayer>(
      overlay_layer->GetId(),        //
      std::move(android_surface),    //
      std::move(surface)             //
    );
    layer->gr_context = gr_context;
    layers_.push_back(layer);
  }

  std::shared_ptr<OverlayLayer> layer = layers_[available_layer_index_];
  if (gr_context != layer->gr_context) {
    layer->gr_context = gr_context;
    // The overlay already exists, but the GrContext was changed so we need to recreate
    // the rendering surface with the new GrContext.
    std::unique_ptr<Surface> surface = layer->android_surface->CreateGPUSurface(gr_context);
    layer->surface = std::move(surface);
  }
  available_layer_index_++;
  return layer;
}

void Pool::RecycleLayers() {
  available_layer_index_ = 0;
}

std::vector<std::shared_ptr<OverlayLayer>> Pool::GetUnusedLayers() {
  std::vector<std::shared_ptr<OverlayLayer>> results;
  for (size_t i = available_layer_index_; i < layers_.size(); i++) {
    results.push_back(layers_[i]);
  }
  return results;
}

} // namespace platform_view

} // namespace flutter
