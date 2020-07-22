#pragma once

#define VK_USE_PLATFORM_WIN32_KHR

#include "vk_common.h"

struct GLFWwindow; // Fwd declare

void initialiseVulkan(GLFWwindow *winPtr);

VmaAllocator createVMAallocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);

VkSemaphore createSemaphore(VkDevice device);

static VkRenderPass createRenderPass(VkDevice device, VkFormat colorFormat, VkFormat depthFormat);

static VkFramebuffer createFramebuffer(VkDevice device, VkRenderPass renderPass, VkImageView colorView,
                                       VkImageView depthView, u32 width, u32 height);

static VkCommandPool createCommandPool(VkDevice device, u32 familyIndex);

void allocateCommandBuffer(VkDevice device, VkCommandPool pool, VkCommandBuffer *cmdBuffer);

void uploadVertices(u32 vbSize, u32 offset, const void *data);

void uploadIndices(u32 ibSize, u32 offset, const void *data);

void createStaticBuffers(u32 vbSize, u32 ibSize);

void uploadUniformData(glm::mat4 view, glm::mat4 proj);
void uploadModelMatrix(glm::mat4 model);

u32 avk_prepareFrame(f64 time);

void avk_drawMesh(u32 vertexOffset, u32 indexOffset, u32 indexCount, f64 time);

void avk_endFrame();