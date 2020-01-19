// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/vulkan/vulkan_native_surface_fuchsia.h"

#include "third_party/skia/include/gpu/vk/GrVkBackendContext.h"

namespace vulkan {

VulkanNativeSurfaceFuchsia::VulkanNativeSurfaceFuchsia() {}

const char* VulkanNativeSurfaceFuchsia::GetExtensionName() const {
  return VK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
}

uint32_t VulkanNativeSurfaceFuchsia::GetSkiaExtensionName() const {
  return kKHR_android_surface_GrVkExtensionFlag;
}

VkSurfaceKHR VulkanNativeSurfaceFuchsia::CreateSurfaceHandle(
    VulkanProcTable& vk,
    const VulkanHandle<VkInstance>& instance) const {
  if (!vk.IsValid() || !instance) {
    return VK_NULL_HANDLE;
  }

  const VkAndroidSurfaceCreateInfoKHR create_info = {
      .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
      .pNext = nullptr,
      .flags = 0,
      .window = native_window_,
  };

  VkSurfaceKHR surface = VK_NULL_HANDLE;

  if (VK_CALL_LOG_ERROR(vk.CreateAndroidSurfaceKHR(
          instance, &create_info, nullptr, &surface)) != VK_SUCCESS) {
    return VK_NULL_HANDLE;
  }

  return surface;
}

bool VulkanNativeSurfaceFuchsia::IsValid() const {
  return true;
}

SkISize VulkanNativeSurfaceFuchsia::GetSize() const {
  return SkISize::Make(0, 0);
}

}  // namespace vulkan
