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

#ifndef NUM_LIGHTS
#define NUM_LIGHTS 2
#endif

#ifndef NUM_PUSH_CONSTANT_MAT4
#define NUM_PUSH_CONSTANT_MAT4 2
#endif

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
    glm::mat4 view;
    glm::mat4 proj;
};

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


struct PushConstants_t {
    //glm::mat4 mat4_pushConst[NUM_PUSH_CONSTANT_MAT4];
    glm::mat4 model;
    glm::vec4 lights[NUM_LIGHTS];
};

extern VulkanContext_t vk_context;
extern GPUInfo_t vk_gpu;
extern Swapchain_t vk_swapchain;

extern VkDevice vk_device;
extern VkQueue vk_queue;
extern VmaAllocator vk_vma;
extern VkFormat vk_swapchainFormat, vk_depthFormat;
extern VkSemaphore vk_acquireSemaphore;
extern VkSemaphore vk_releaseSemaphore;
extern VkRenderPass vk_renderPass;
extern VkFramebuffer vk_targetFramebuffer;
extern VkCommandPool vk_commandPool;
extern VkCommandBuffer vk_commandBuffer;
extern VkDescriptorPool vk_descPool;
extern VkDescriptorSetLayout vk_descSetLayout;
extern VkDescriptorSet vk_descSets[1];
extern VkPipelineCache vk_pipelineCache;
extern VkPipelineLayout vk_gfxPipeLayout;
extern VkPipeline vk_meshPipeline ;

extern Image_t vk_colorTarget;
extern Image_t vk_depthTarget;

extern Buffer_t vk_staticVertexBuffer;
extern Buffer_t vk_staticIndexBuffer;
extern Buffer_t vk_uniformBuffer;

extern Uniforms_t vk_uniformData;
extern PushConstants_t vk_pushConstants;

extern Shader_t vk_meshVS;
extern Shader_t vk_goochFS;
extern Shader_t vk_lambertFS;


