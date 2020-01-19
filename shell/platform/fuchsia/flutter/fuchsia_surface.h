// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_FUCHSIA_FLUTTER_FUCHSIA_SURFACE_H_
#define FLUTTER_SHELL_PLATFORM_FUCHSIA_FLUTTER_FUCHSIA_SURFACE_H_

#include <fuchsia/ui/gfx/cpp/fidl.h>
#include <fuchsia/ui/views/cpp/fidl.h>
#include <memory>

#include "flutter/fml/macros.h"
#include "flutter/shell/common/surface.h"
#include "third_party/skia/include/core/SkSize.h"

namespace flutter {

class FuchsiaSurface {
 public:
  virtual ~FuchsiaSurface() = default;

  virtual fuchsia::ui::views::ViewRef view_ref() = 0;
  virtual const zx_handle_t& vsync_event_handle() = 0;

  virtual bool IsValid() const = 0;

  // Called on the platform thread to create a surface for use by the GPU
  // thread.
  virtual std::unique_ptr<Surface> CreateGPUSurface() = 0;

  // Notification callbacks.  Must be called on the GPU thread.
  virtual void OnChildViewConnected(scenic::ResourceId view_holder_id) = 0;
  virtual void OnChildViewDisconnected(scenic::ResourceId view_holder_id) = 0;
  virtual void OnChildViewStateChanged(scenic::ResourceId view_holder_id, bool state) = 0;
  virtual void OnMetricsUpdate(const fuchsia::ui::gfx::Metrics& metrics) = 0;
  virtual void OnSizeChangeHint(const SkSize& size_scale) const = 0;
  virtual void OnEnableWireframe(bool enable) const = 0;
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_FUCHSIA_FLUTTER_FUCHSIA_SURFACE_H_
