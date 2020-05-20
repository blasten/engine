// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/android/android_surface_gl.h"

#include <utility>

#include "flutter/fml/logging.h"
#include "flutter/fml/memory/ref_ptr.h"

namespace flutter {

static fml::RefPtr<AndroidContextGL> CreateResourceLoadingContext() {
  auto environment = fml::MakeRefCounted<AndroidEnvironmentGL>();
  if (!environment->IsValid()) {
    FML_LOG(ERROR) << "(hybrid) environment isn't valid";

    return nullptr;
  }

  auto context = fml::MakeRefCounted<AndroidContextGL>(environment);
  if (!context->IsValid()) {
    FML_LOG(ERROR) << "(hybrid)  context isn't valid";
    return nullptr;
  }

  return context;
}

AndroidSurfaceGL::AndroidSurfaceGL(fml::jni::JavaObjectWeakGlobalRef java_object) {
  // Acquire the offscreen context.
  offscreen_context_ = CreateResourceLoadingContext();
 
  if (!offscreen_context_ || !offscreen_context_->IsValid()) {
    FML_LOG(ERROR) << "(hybrid) offscreen_context_ is NULL";
    offscreen_context_ = nullptr;
  }

  FML_LOG(ERROR) << "(hybrid) CREATED AndroidSurfaceGL";

  external_view_embedder_ = std::make_unique<AndroidExternalViewEmbedder>(java_object);
}

AndroidSurfaceGL::~AndroidSurfaceGL() = default;

bool AndroidSurfaceGL::IsOffscreenContextValid() const {
  return offscreen_context_ && offscreen_context_->IsValid();
}

void AndroidSurfaceGL::TeardownOnScreenContext() {
  if (onscreen_context_) {
    onscreen_context_->ClearCurrent();
  }
  onscreen_context_ = nullptr;
}

bool AndroidSurfaceGL::IsValid() const {
  if (!onscreen_context_ || !offscreen_context_) {
    return false;
  }

  return onscreen_context_->IsValid() && offscreen_context_->IsValid();
}

std::unique_ptr<Surface> AndroidSurfaceGL::CreateGPUSurface(GrContext* gr_context) {
  if (gr_context) {
    return std::make_unique<GPUSurfaceGL>(sk_ref_sp(gr_context), this, true);
  }
  return std::make_unique<GPUSurfaceGL>(this, true);
}

bool AndroidSurfaceGL::OnScreenSurfaceResize(const SkISize& size) const {
  FML_DCHECK(onscreen_context_ && onscreen_context_->IsValid());
  return onscreen_context_->Resize(size);
}

bool AndroidSurfaceGL::ResourceContextMakeCurrent() {
  FML_DCHECK(offscreen_context_ && offscreen_context_->IsValid());
  return offscreen_context_->MakeCurrent();
}

bool AndroidSurfaceGL::ResourceContextClearCurrent() {
  FML_DCHECK(offscreen_context_ && offscreen_context_->IsValid());
  return offscreen_context_->ClearCurrent();
}

bool AndroidSurfaceGL::SetNativeWindow(
    fml::RefPtr<AndroidNativeWindow> window) {
  // In any case, we want to get rid of our current onscreen context.
  onscreen_context_ = nullptr;

  // If the offscreen context has not been setup, we dont have the sharegroup.
  // So bail.
  if (!offscreen_context_ || !offscreen_context_->IsValid()) {
    FML_LOG(ERROR) << "(hybrid) offscreen_context_ isn't valid";
    return false;
  }

  // Create the onscreen context.
  onscreen_context_ = fml::MakeRefCounted<AndroidContextGL>(
      offscreen_context_->Environment(),
      offscreen_context_.get() /* sharegroup */);

  if (!onscreen_context_->IsValid()) {
    FML_LOG(ERROR) << "(hybrid) on screen isn't valid";
    onscreen_context_ = nullptr;
    return false;
  }

  if (!onscreen_context_->CreateWindowSurface(std::move(window))) {
    FML_LOG(ERROR) << "(hybrid) cannot create CreateWindowSurface";

    onscreen_context_ = nullptr;
    return false;
  }

  FML_LOG(ERROR) << "(hybrid) SetNativeWindow";
  return true;
}

bool AndroidSurfaceGL::GLContextMakeCurrent() {
  FML_LOG(ERROR) << "(hybrid) AndroidSurfaceGL::GLContextMakeCurrent";
  FML_DCHECK(onscreen_context_);
  FML_DCHECK(onscreen_context_->IsValid());
  return onscreen_context_->MakeCurrent();
}

bool AndroidSurfaceGL::GLContextClearCurrent() {
  FML_DCHECK(onscreen_context_ && onscreen_context_->IsValid());
  return onscreen_context_->ClearCurrent();
}

bool AndroidSurfaceGL::GLContextPresent() {
  FML_DCHECK(onscreen_context_ && onscreen_context_->IsValid());
  return onscreen_context_->SwapBuffers();
}

intptr_t AndroidSurfaceGL::GLContextFBO() const {
  FML_DCHECK(onscreen_context_ && onscreen_context_->IsValid());
  // The default window bound framebuffer on Android.
  return 0;
}

// |GPUSurfaceGLDelegate|
ExternalViewEmbedder* AndroidSurfaceGL::GetExternalViewEmbedder() {
  return external_view_embedder_.get();
}

}  // namespace flutter
