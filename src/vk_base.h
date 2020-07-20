#pragma once

#define VK_USE_PLATFORM_WIN32_KHR

#include "vk_common.h"

struct GLFWwindow; // Fwd declare

void initialiseVulkan(GLFWwindow *winPtr);

VmaAllocator createVMAallocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);

VkSemaphore createSemaphore(VkDevice device);

static VkRenderPass
createRenderPass(VkDevice device, VkFormat colorFormat, VkFormat depthFormat);

static VkFramebuffer
createFramebuffer(VkDevice device, VkRenderPass renderPass, VkImageView colorView,
                  VkImageView depthView, u32 width, u32 height);

static
VkCommandPool createCommandPool(VkDevice device, u32 familyIndex);

void allocateCommandBuffer(VkDevice device, VkCommandPool pool, VkCommandBuffer *cmdBuffer);

u32 renderFrame(u32 indexCount, f64 time);

void uploadVertices(u32 vbSize, const void* data);
void uploadIndices(u32 ibSize, const void* data);


void firstRenderPrograms();