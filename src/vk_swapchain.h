#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#define VSYNC 1

struct Swapchain_t
{
    VkSwapchainKHR swapchain;
    std::vector<VkImage> images;
    u32 width, height;
    u32 imageCount;
};

enum SwapchainStatus_t
{
    Swapchain_Ready,
    Swapchain_Resized,
    Swapchain_NotReady,
};

extern Swapchain_t swapchain;

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