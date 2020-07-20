#include "vk_base.h"
#include "vk_device.h"
#include "vk_surface.h"
#include "vk_swapchain.h"
#include "vk_resources.h"
#include "vk_render.h"
#include "vk_renderprograms.h"

VulkanContext_t vk_context;
GPUInfo_t gpu;
VkDevice vk_device;
VkQueue queue = 0;
VmaAllocator vma;
VkFormat swapchainFormat, depthFormat;
VkSemaphore acquireSemaphore;
VkSemaphore releaseSemaphore;
Swapchain_t swapchain;
VkRenderPass renderPass;
VkCommandPool commandPool = 0;
VkCommandBuffer commandBuffer = 0;

u32 imageIndex = 0;

Buffer_t vb = {};
Buffer_t ib = {};
Buffer_t uboBuffer = {};

Uniforms_t uboVS;

std::vector<glm::vec4> pushConstants; //NOTE(anton): vec4 instead of vec3 for alignment requirements

u32 renderFrame(u32 indexCount, f64 time) {
    imageIndex = prepareFrame(uboVS, pushConstants, time);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vb.buffer, &offset);
    vkCmdBindIndexBuffer(commandBuffer, ib.buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            gfxPipeLayout, 0, 1, descSets, 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);

    submitFrame(imageIndex);

    return imageIndex;
}

void initialiseVulkan(GLFWwindow *windowPtr) {
    vk_context.instance = createInstance();

#ifdef _DEBUG
    setupDebugMessenger(vk_context.instance, &vk_context.debugMessenger);
#endif

    createGLFWsurface(windowPtr, vk_context);

    gpu = pickGPU(vk_context.instance, vk_context.surface);

    vk_device = createDevice(vk_context.instance, &gpu);

    vma = createVMAallocator(vk_context.instance, gpu.device, vk_device);

    swapchainFormat = getSwapchainFormat(gpu.device, vk_context.surface);
    depthFormat = VK_FORMAT_D32_SFLOAT;

    vkGetDeviceQueue(vk_device, gpu.gfxFamilyIndex, 0, &queue);

    acquireSemaphore = createSemaphore(vk_device);
    releaseSemaphore = createSemaphore(vk_device);

    createSwapchain(swapchain, gpu.device, vk_device, vk_context.surface,
                    swapchainFormat, gpu.gfxFamilyIndex, /*oldSwapchain=*/VK_NULL_HANDLE);

    renderPass = createRenderPass(vk_device, swapchainFormat, depthFormat);

    commandPool = createCommandPool(vk_device, gpu.gfxFamilyIndex);
    allocateCommandBuffer(vk_device, commandPool, &commandBuffer);

    glm::vec4 initLight1Pos = glm::vec4(-1.0f, 1.0f, 8.0f, 1.0f);
    glm::vec4 initLight2Pos = glm::vec4(0.0f, 1.0f, -3.0f, 1.0f);
    pushConstants.push_back(initLight1Pos);
    pushConstants.push_back(initLight2Pos);

    createBuffer(uboBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(uboVS), vma);

    glm::vec3 initCameraPos = glm::vec3(4.0f, 3.0f, 7.0f);

    glm::mat4 initView = glm::lookAt(initCameraPos, // eye
                                     glm::vec3(0.0f, 0.0f, 0.0f), // center
                                     glm::vec3(0.0f, 1.0f, 0.0f)); // up

    uboVS.model = glm::mat4(1.0f);
    uboVS.view = initView;

}

void firstRenderPrograms() {
    setupFirstTimeRenderprogs(sizeof(uboVS), pushConstants);
}

void uploadVertices(u32 vbSize, const void *data) {
    Buffer_t vb_staging = {};
    createBuffer(vb,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VMA_MEMORY_USAGE_GPU_ONLY,
                 vbSize, vma);
    createBuffer(vb_staging,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VMA_MEMORY_USAGE_CPU_ONLY,
                 vbSize, vma);
    uploadBuffer(vk_device, commandPool, commandBuffer, queue,
                 vb, vb_staging,
                 data, vbSize, vma);
}

void uploadIndices(u32 ibSize, const void *data) {
        Buffer_t ib_staging = {};
        createBuffer(ib,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VMA_MEMORY_USAGE_GPU_ONLY,
                     ibSize, vma);
        createBuffer(ib_staging,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VMA_MEMORY_USAGE_CPU_ONLY,
                     ibSize, vma);
        uploadBuffer(vk_device, commandPool, commandBuffer, queue,
                     ib, ib_staging,
                     data, ibSize, vma);
}

void setVulkanUBORef(Uniforms_t uboRef) {
    Logger::Trace("entered setVulkanUBORef");
    uboVS.model = glm::mat4(1.0f);
    Logger::Trace("uboVS.model = glm::mat4(1.0f);");

    uboVS.view = uboRef.view;
    Logger::Trace("uboVS.view = uboRef.view;");

    uboVS.proj = uboRef.proj;
    Logger::Trace("uboVS.proj = uboRef.proj;");

    Logger::Trace("uboVS.view[0][1], [1][2], [2][3]: %f %f %f", uboVS.view[0][1], uboVS.view[1][2], uboVS.view[2][3]);
    Logger::Trace("uboVS.proj[0][1], [1][2], [2][3]: %f %f %f", uboVS.proj[0][1], uboVS.proj[1][2], uboVS.proj[2][3]);


    void *data;
    vmaMapMemory(vma, uboBuffer.vmaAlloc, &data);
    memcpy(data, &uboVS, sizeof(uboVS));
    vmaUnmapMemory(vma, uboBuffer.vmaAlloc);
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
    VK_CHECK( vkCreateRenderPass(device, &createInfo, nullptr, &rp) );
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