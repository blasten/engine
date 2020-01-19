// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/fuchsia/flutter/fuchsia_surface_vulkan.h"

#include "flutter/fml/logging.h"
#include "flutter/shell/gpu/gpu_surface_vulkan.h"
#include "flutter/shell/platform/embedder/embedder.h"
//#include "flutter/vulkan/vulkan_native_surface_fuchsia.h"

namespace flutter {

FuchsiaSurfaceVulkan::FuchsiaSurfaceVulkan()
    : proc_table_(fml::MakeRefCounted<vulkan::VulkanProcTable>()),
      external_view_embedder_([](GrContext* context, const FlutterBackingStoreConfig& config) -> std::unique_ptr<EmbedderRenderTarget> {
        return nullptr;
      },
      [](const std::vector<const FlutterLayer*>& layers) -> bool {
        return true;
      }) {}

FuchsiaSurfaceVulkan::~FuchsiaSurfaceVulkan() = default;

bool FuchsiaSurfaceVulkan::IsValid() const {
  return proc_table_->HasAcquiredMandatoryProcAddresses();
}

std::unique_ptr<Surface> FuchsiaSurfaceVulkan::CreateGPUSurface() {
  if (!IsValid()) {
    return nullptr;
  }

  auto vulkan_surface_fuchsia =
      std::make_unique<vulkan::VulkanNativeSurfaceFuchsia>();

  if (!vulkan_surface_fuchsia->IsValid()) {
    return nullptr;
  }

  auto gpu_surface = std::make_unique<GPUSurfaceVulkan>(
      proc_table_, std::move(vulkan_surface_fuchsia));

  if (!gpu_surface->IsValid()) {
    return nullptr;
  }

  return gpu_surface;
}

}  // namespace flutter
