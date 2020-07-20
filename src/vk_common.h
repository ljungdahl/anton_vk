#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../external/vk_mem_alloc.h"
#include "typedefs.h"
#include "logger.h"

#include "anton_asserts.h"

#define VK_CHECK(expr) { \
    ASSERT(expr == VK_SUCCESS); \
}


struct VulkanContext_t {
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR surface = 0;
};

struct GPUInfo_t {
    VkPhysicalDevice device = VK_NULL_HANDLE;
    VkPhysicalDeviceFeatures features = {};
    VkPhysicalDeviceProperties props = {};
    VkPhysicalDeviceMemoryProperties memProps = {};
    u32 gfxFamilyIndex = U32_MAX;
    u32 presentFamilyIndex = U32_MAX;
};

struct Buffer_t {
    VkBuffer buffer;
    VmaAllocation vmaAlloc;
    VmaAllocationInfo vmaInfo;
    size_t size;
};

struct Image_t {
    VkImage image;
    VkImageView view;
    VmaAllocation vmaAlloc;
    VmaAllocationInfo vmaInfo;
};

struct Shader_t {
    VkShaderModule module;
    VkShaderStageFlagBits stage;
};

struct Uniforms_t {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

extern VulkanContext_t vk_context;
extern GPUInfo_t gpu;
extern VkDevice vk_device;
extern VkQueue queue;
extern VmaAllocator vma;
extern VkFormat swapchainFormat, depthFormat;
extern VkSemaphore acquireSemaphore;
extern VkSemaphore releaseSemaphore;
extern VkRenderPass renderPass;
extern VkCommandPool commandPool;
extern VkCommandBuffer commandBuffer;
extern Buffer_t vb;
extern Buffer_t ib;
extern Buffer_t uboBuffer;