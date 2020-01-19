// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_VULKAN_VULKAN_NATIVE_SURFACE_FUCHSIA_H_
#define FLUTTER_VULKAN_VULKAN_NATIVE_SURFACE_FUCHSIA_H_

#include "flutter/fml/macros.h"
#include "flutter/vulkan/vulkan_native_surface.h"

namespace vulkan {

class VulkanNativeSurfaceFuchsia : public VulkanNativeSurface {
 public:
  VulkanNativeSurfaceFuchsia();
  ~VulkanNativeSurfaceFuchsia() override = default;

  const char* GetExtensionName() const override;

  uint32_t GetSkiaExtensionName() const override;

  VkSurfaceKHR CreateSurfaceHandle(
      VulkanProcTable& vk,
      const VulkanHandle<VkInstance>& instance) const override;

  bool IsValid() const override;

  SkISize GetSize() const override;

 private:
  FML_DISALLOW_COPY_AND_ASSIGN(VulkanNativeSurfaceFuchsia);
};

}  // namespace vulkan

#endif  // FLUTTER_VULKAN_VULKAN_NATIVE_SURFACE_FUCHSIA_H_
