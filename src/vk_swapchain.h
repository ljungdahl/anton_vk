#pragma once

#include "vk_common.h"

#define VSYNC 1

VkFormat getSwapchainFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

VkFormat getDepthFormat(VkPhysicalDevice physicalDevice);

void destroySwapchain(VkDevice device, const Swapchain_t &swapchain);

void createSwapchain(Swapchain_t &result, VkPhysicalDevice physicalDevice,
                     VkDevice device, VkSurfaceKHR surface,
                     VkFormat format, u32 familyIndex, VkSwapchainKHR oldSwapchain);

VkSwapchainKHR createSwapchainKHR(VkDevice device, VkSurfaceKHR surface,
                                  VkSurfaceCapabilitiesKHR surfaceCaps,
                                  VkFormat format, u32 width, u32 height,
                                  u32 familyIndex, VkSwapchainKHR oldSwapchain);

SwapchainStatus_t updateSwapchain(Swapchain_t &result, VkPhysicalDevice physicalDevice,
                                  VkDevice device, VkSurfaceKHR surface, VkFormat format,
                                  u32 familyIndex);