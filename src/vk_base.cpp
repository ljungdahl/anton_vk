#include "vk_base.h"
#include "vk_device.h"
#include "vk_surface.h"
#include "vk_swapchain.h"
#include "vk_resources.h"
#include "vk_render.h"
#include "vk_renderprograms.h"

VulkanContext_t vk_context;
GPUInfo_t vk_gpu;
Swapchain_t vk_swapchain;

VkDevice vk_device;
VkQueue vk_queue = 0;
VmaAllocator vk_vma;
VkFormat vk_swapchainFormat, vk_depthFormat;
VkSemaphore vk_acquireSemaphore;
VkSemaphore vk_releaseSemaphore;

VkRenderPass vk_renderPass;
VkCommandPool vk_commandPool = 0;
VkCommandBuffer vk_commandBuffer = 0;

Buffer_t vk_staticVertexBuffer = {};
Buffer_t vk_staticIndexBuffer = {};
Buffer_t vk_uniformBuffer = {};

u32 vk_imageIndex = 0;

Uniforms_t vk_uniformData = {};

PushConstants_t vk_pushConstants;

void initialiseVulkan(GLFWwindow *windowPtr) {
    vk_context.instance = createInstance();

#ifdef _DEBUG
    setupDebugMessenger(vk_context.instance, &vk_context.debugMessenger);
#endif

    createGLFWsurface(windowPtr, vk_context);

    vk_gpu = pickGPU(vk_context.instance, vk_context.surface);

    vk_device = createDevice(vk_context.instance, &vk_gpu);

    vk_vma = createVMAallocator(vk_context.instance, vk_gpu.device, vk_device);

    vk_swapchainFormat = getSwapchainFormat(vk_gpu.device, vk_context.surface);
    vk_depthFormat = VK_FORMAT_D32_SFLOAT;

    vkGetDeviceQueue(vk_device, vk_gpu.gfxFamilyIndex, 0, &vk_queue);

    vk_acquireSemaphore = createSemaphore(vk_device);
    vk_releaseSemaphore = createSemaphore(vk_device);

    createSwapchain(vk_swapchain, vk_gpu.device, vk_device, vk_context.surface,
                    vk_swapchainFormat, vk_gpu.gfxFamilyIndex, /*oldSwapchain=*/VK_NULL_HANDLE);

    vk_renderPass = createRenderPass(vk_device, vk_swapchainFormat, vk_depthFormat);

    vk_commandPool = createCommandPool(vk_device, vk_gpu.gfxFamilyIndex);
    allocateCommandBuffer(vk_device, vk_commandPool, &vk_commandBuffer);

    vk_pushConstants.model = glm::mat4(1.0f);
    Logger::Trace("sizeof(vk_pushConstants) %i", sizeof(vk_pushConstants));

    glm::vec4 initLight1Pos = glm::vec4(-1.0f, 1.0f, 8.0f, 1.0f);
    glm::vec4 initLight2Pos = glm::vec4(0.0f, 1.0f, -3.0f, 1.0f);
    vk_pushConstants.lights[0] = initLight1Pos;
    vk_pushConstants.lights[1] = initLight2Pos;

    glm::vec3 initCameraPos = glm::vec3(4.0f, 3.0f, 7.0f);

    glm::mat4 initView = glm::lookAt(initCameraPos, // eye
                                     glm::vec3(0.0f, 0.0f, 0.0f), // center
                                     glm::vec3(0.0f, 1.0f, 0.0f)); // up

    vk_uniformData.view = initView;

    // Uniform buffer
    createBuffer(vk_uniformBuffer,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VMA_MEMORY_USAGE_CPU_TO_GPU,
                 sizeof(vk_uniformData), vk_vma);

    initialDescriptorSetup();
    initialShaderLoad();
    initialPipelineCreation();
}

u32 avk_prepareFrame(f64 time) {

    vk_imageIndex = prepareFrame();
    return vk_imageIndex;
}

void avk_drawMesh(u32 vertexOffset, u32 indexOffset, u32 indexCount, f64 time) {

    drawMesh(vertexOffset, indexOffset, indexCount, time);
}

void avk_endFrame() {
    submitFrame(vk_imageIndex);
}

void createStaticBuffers(u32 vbSize, u32 ibSize) {
    createBuffer(vk_staticVertexBuffer,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VMA_MEMORY_USAGE_GPU_ONLY,
                 vbSize, vk_vma);

    Logger::Trace("Created static vertexbuffer of size %i", vbSize);

    createBuffer(vk_staticIndexBuffer,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VMA_MEMORY_USAGE_GPU_ONLY,
                 ibSize, vk_vma);

    Logger::Trace("Created static indexbuffer of size %i", ibSize);

}

void uploadUniformData(glm::mat4 view, glm::mat4 proj) {

    vk_uniformData.view = view;

    vk_uniformData.proj = proj;

    updateUniforms();
}

void uploadModelMatrix(glm::mat4 model) {
    vk_pushConstants.model = model;
}

void uploadVertices(u32 vbSize, u32 offset, const void *data) {
    Buffer_t vb_staging = {};

    ASSERT(vk_staticVertexBuffer.buffer != VK_NULL_HANDLE);

    createBuffer(vb_staging,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VMA_MEMORY_USAGE_CPU_ONLY,
                 vbSize, vk_vma);

    Logger::Trace("Created static staging vertex buffer of size %i", vbSize);

    uploadBuffer(vk_device, vk_commandPool, vk_commandBuffer, vk_queue,
                 vk_staticVertexBuffer, vb_staging,
                 data, offset, vbSize, vk_vma);

    Logger::Trace("Uploaded static vertex staging buffer of size %i at offset %i",
                  vbSize, offset);

    vmaDestroyBuffer(vk_vma, vb_staging.buffer, vb_staging.vmaAlloc);
    Logger::Trace("Destroyed static staging vertex buffer");
}

void uploadIndices(u32 ibSize, u32 offset, const void *data) {
    Buffer_t ib_staging = {};

    ASSERT(vk_staticIndexBuffer.buffer != VK_NULL_HANDLE);

    createBuffer(ib_staging,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VMA_MEMORY_USAGE_CPU_ONLY,
                 ibSize, vk_vma);

    Logger::Trace("Created static staging index buffer of size %i at offset %i",
                  ibSize, offset);

    uploadBuffer(vk_device, vk_commandPool, vk_commandBuffer, vk_queue,
                 vk_staticIndexBuffer, ib_staging,
                 data, offset, ibSize, vk_vma);

    Logger::Trace("Uploaded static index staging buffer of size %i", ibSize);

    vmaDestroyBuffer(vk_vma, ib_staging.buffer, ib_staging.vmaAlloc);

    Logger::Trace("Destroyed static staging index buffer");
}


static
VmaAllocator createVMAallocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device) {
    VmaAllocatorCreateInfo vmaAllocCI = {};
    vmaAllocCI.physicalDevice = physicalDevice;
    vmaAllocCI.device = device;
    vmaAllocCI.instance = instance;

    VmaAllocator vma_allocator = 0;
    vmaCreateAllocator(&vmaAllocCI, &vma_allocator);
    return vma_allocator;
}

static
VkSemaphore createSemaphore(VkDevice device) {
    VkSemaphoreCreateInfo createInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    VkSemaphore semaphore = 0;
    VK_CHECK(vkCreateSemaphore(device, &createInfo, nullptr, &semaphore));

    return semaphore;
}

static VkRenderPass
createRenderPass(VkDevice device, VkFormat colorFormat, VkFormat depth_format) {

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = 0;

    VkAttachmentDescription attachments[2] = {};
    attachments[0].format = colorFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // NOTE(anton): Remember to have this on clear if you want a clear color on color attachment...
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    attachments[1].format = depth_format;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkRenderPassCreateInfo createInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    createInfo.attachmentCount = ARRAYSIZE(attachments);
    createInfo.pAttachments = attachments;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;

    VkRenderPass rp = 0;
    VK_CHECK(vkCreateRenderPass(device, &createInfo, nullptr, &rp));
    return rp;
}

static
VkCommandPool createCommandPool(VkDevice device, u32 familyIndex) {
    VkCommandPoolCreateInfo createInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    createInfo.queueFamilyIndex = familyIndex;

    VkCommandPool cmdPool = 0;
    VK_CHECK(vkCreateCommandPool(device, &createInfo, nullptr, &cmdPool));

    return cmdPool;
}

void allocateCommandBuffer(VkDevice device, VkCommandPool pool, VkCommandBuffer *cmdBuffer) {
    VkCommandBufferAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, cmdBuffer));
}