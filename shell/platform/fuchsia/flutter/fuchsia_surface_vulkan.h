// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_FUCHSIA_FLUTTER_FUCHSIA_SURFACE_VULKAN_H_
#define FLUTTER_SHELL_PLATFORM_FUCHSIA_FLUTTER_FUCHSIA_SURFACE_VULKAN_H_

#include <fuchsia/ui/gfx/cpp/fidl.h>
#include <fuchsia/ui/views/cpp/fidl.h>
#include <lib/ui/scenic/cpp/resources.h>

#include "flutter/fml/macros.h"
#include "flutter/fml/memory/ref_ptr.h"
#include "flutter/shell/platform/embedder/embedder_external_view_embedder.h"
#include "flutter/shell/platform/fuchsia/flutter/fuchsia_surface.h"
#include "flutter/vulkan/vulkan_proc_table.h"

namespace flutter {

class FuchsiaSurfaceVulkan : public FuchsiaSurface {
 public:
  FuchsiaSurfaceVulkan();
  ~FuchsiaSurfaceVulkan() override;

  // |FuchsiaSurface|
  fuchsia::ui::views::ViewRef view_ref() override;
  
  // |FuchsiaSurface|
  const zx_handle_t& vsync_event_handle()) override { return ZX_HANDLE_INVALID; }

  // |FuchsiaSurface|
  bool IsValid() const override;

  // |FuchsiaSurface|
  std::unique_ptr<Surface> CreateGPUSurface() override;

  // |FuchsiaSurface|
  void OnChildViewConnected(scenic::ResourceId view_holder_id) override {}

  // |FuchsiaSurface|
  void OnChildViewDisconnected(scenic::ResourceId view_holder_id) override {}

  // |FuchsiaSurface|
  void OnChildViewStateChanged(scenic::ResourceId view_holder_id, bool state) override {}

  // |FuchsiaSurface|
  void OnMetricsUpdate(const fuchsia::ui::gfx::Metrics& metrics) override {}

  // |FuchsiaSurface|
  void OnSizeChangeHint(const SkSize& size_scale) const override {}

  // |FuchsiaSurface|
  void OnEnableWireframe(bool enable) const override {}

 private:
  fml::RefPtr<vulkan::VulkanProcTable> proc_table_;
  // FuchsiaScenicCompositor scenic_compositor_;
  EmbedderExternalViewEmbedder external_view_embedder_;

  FML_DISALLOW_COPY_AND_ASSIGN(FuchsiaSurfaceVulkan);
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_FUCHSIA_FLUTTER_FUCHSIA_SURFACE_VULKAN_H_
