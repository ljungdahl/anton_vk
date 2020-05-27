#include "common.h"
#include "device.cpp"
#include "swapchain.cpp"
#include "resources.cpp"

// --------------------------------------------------------------
// Forward declare
void processKeyInput(GLFWwindow* windowPtr);

// --------------------------------------------------------------

static
VmaAllocator createVMAallocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device)
{
    VmaAllocatorCreateInfo vmaAllocCI = {};
    vmaAllocCI.physicalDevice = physicalDevice;
    vmaAllocCI.device = device;
    vmaAllocCI.instance = instance;

    VmaAllocator vma_allocator = 0;
    vmaCreateAllocator(&vmaAllocCI, &vma_allocator);
    return vma_allocator;
}

VkCommandPool createCommandPool(VkDevice device, u32 familyIndex)
{
    VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    createInfo.queueFamilyIndex = familyIndex;

    VkCommandPool commandPool = 0;
    VK_CHECK( vkCreateCommandPool(device, &createInfo, nullptr, &commandPool) );

    return commandPool;
    
}

void allocateCommandBuffer(VkDevice device, VkCommandPool pool, VkCommandBuffer* cmdBuffer)
{
    VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocInfo.commandPool = pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VK_CHECK( vkAllocateCommandBuffers(device, &allocInfo, cmdBuffer) );
}

VkRenderPass createRenderPass(VkDevice device, VkFormat colorFormat)
{

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = 0;

    VkAttachmentDescription attachments[1] = {};
    attachments[0].format = colorFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // NOTE(anton): Remember to have this on clear if you want a clear color on color attachment...
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;//VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    createInfo.attachmentCount = ARRAYSIZE(attachments);
    createInfo.pAttachments = attachments;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;

    VkRenderPass renderPass = 0;
    VK_CHECK( vkCreateRenderPass(device, &createInfo, nullptr, &renderPass) );
    return renderPass;
}

VkFramebuffer createFramebuffer(VkDevice device, VkRenderPass renderPass, VkImageView imageView, u32 width, u32 height)
{
    VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    createInfo.renderPass = renderPass;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments = &imageView;
    createInfo.width = width;
    createInfo.height = height;
    createInfo.layers = 1;

    VkFramebuffer fb = 0;
    VK_CHECK( vkCreateFramebuffer(device, &createInfo, nullptr, &fb) );

    return fb;
}

VkSemaphore createSemaphore(VkDevice device)
{
    VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    VkSemaphore semaphore = 0;
    VK_CHECK(vkCreateSemaphore(device, &createInfo, 0, &semaphore));

    return semaphore;
}

static
VkFence createWaitFence(VkDevice device)
{    
    VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VkFence waitFence = VK_NULL_HANDLE;
    VK_CHECK( vkCreateFence(device, &fenceCreateInfo, nullptr, &waitFence) );
    return waitFence;
}

// --------------------------------------------------------------

i32 main(i32 argc, const i8** argv)
{    
#ifdef _DEBUG
    Logger::Trace("_DEBUG defined.");
#endif
#ifdef _WIN32
    Logger::Trace("_WIN32 defined.");
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
    Logger::Trace("VK_USE_PLATFORM_WIN32_KHR defined.");
#endif
    
    i32 rc = glfwInit();
    ASSERT(rc == GLFW_TRUE);
    ASSERT(glfwVulkanSupported() == GLFW_TRUE);
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    
    GLFWwindow* windowPtr;
    windowPtr = glfwCreateWindow(1280, 720, "anton_vk", nullptr, nullptr);

    VkInstance instance = createInstance();
    
#ifdef _DEBUG
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    setupDebugMessenger(instance, &debugMessenger);
#endif

    VkSurfaceKHR surface = 0;
    VK_CHECK( glfwCreateWindowSurface(instance, windowPtr, nullptr, &surface) );
        
    GPUInfo_t gpu = pickGPU(instance, surface);

    VkDevice device = createDevice(instance, &gpu);

    VmaAllocator vma = createVMAallocator(instance, gpu.device, device);
    
    VkFormat colorFormat = getSwapchainFormat(gpu.device, surface);
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

    VkQueue queue = 0;
    vkGetDeviceQueue(device, gpu.gfxFamilyIndex, 0, &queue);
              
    VkSemaphore acquireSemaphore = createSemaphore(device);
    VkSemaphore releaseSemaphore = createSemaphore(device);
    // VkFence waitFence = createWaitFence(device); // NOTE(anton): is this even needed?

    Swapchain_t swapchain = {};
    createSwapchain(swapchain, gpu.device, device, surface, colorFormat, /*oldSwapchain=*/VK_NULL_HANDLE);

    VkRenderPass renderPass = createRenderPass(device, colorFormat);

    VkCommandPool commandPool = createCommandPool(device, gpu.gfxFamilyIndex);
    VkCommandBuffer commandBuffer = 0;
    allocateCommandBuffer(device, commandPool, &commandBuffer);

    VkFramebuffer framebuffers[3];
    VkImageView swapchainView[3];
    ASSERT(swapchain.imageCount <= 3 && swapchain.imageCount > 0);
    for(u32 i = 0; i < swapchain.imageCount; i++)
    {
        swapchainView[i] = createImageView(device, swapchain.images[i], colorFormat);
        framebuffers[i] = createFramebuffer(device, renderPass, swapchainView[i], swapchain.width, swapchain.height);
    }


    u32 imageIndex = 0;
    u32 frameCounter = 0;
    while(!glfwWindowShouldClose(windowPtr))
    {
        glfwPollEvents();

        processKeyInput(windowPtr);
        
        VK_CHECK( vkAcquireNextImageKHR(device, swapchain.swapchain, U64_MAX, acquireSemaphore, /*fence=*/VK_NULL_HANDLE, &imageIndex) ); 
        //Logger::Trace("Image index: %i", imageIndex);
        
        VK_CHECK( vkResetCommandPool(device, commandPool, 0) );

        VkCommandBufferBeginInfo cmdBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK( vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo) );

        VkClearColorValue color = { 1, 0, 1, 1 }; // MAGENTA
        VkClearValue clearVals[1];
        clearVals[0].color = color;
        
        VkRenderPassBeginInfo rpBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        rpBeginInfo.renderPass = renderPass;
        rpBeginInfo.framebuffer = framebuffers[imageIndex];
        rpBeginInfo.renderArea.offset.x = 0;
        rpBeginInfo.renderArea.offset.y = 0;
        rpBeginInfo.renderArea.extent.width = swapchain.width;
        rpBeginInfo.renderArea.extent.height = swapchain.height;
        rpBeginInfo.clearValueCount = ARRAYSIZE(clearVals);
        rpBeginInfo.pClearValues = clearVals;

        vkCmdBeginRenderPass(commandBuffer, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);


        vkCmdEndRenderPass(commandBuffer);
        
        VK_CHECK( vkEndCommandBuffer(commandBuffer) );
        
        VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };    
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &acquireSemaphore;
        submitInfo.pWaitDstStageMask = &waitDstStageMask;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &releaseSemaphore;

        vkQueueSubmit(queue, 1, &submitInfo, /*fence*/VK_NULL_HANDLE);

        VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &releaseSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain.swapchain;
        presentInfo.pImageIndices = &imageIndex;

        VK_CHECK( vkQueuePresentKHR(queue, &presentInfo) );

        VK_CHECK( vkDeviceWaitIdle(device) );

        frameCounter++;
        char title[256];
        sprintf(title, "frame: %i - imageIndex: %i - ", frameCounter, imageIndex);
        glfwSetWindowTitle(windowPtr, title);
    }
    
    if(windowPtr)
    {
        glfwDestroyWindow(windowPtr);
    }
    glfwTerminate();
    
    return 0;   
}

// --------------------------------------------------------------

void processKeyInput(GLFWwindow* windowPtr)
{
    if (glfwGetKey(windowPtr, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(windowPtr, true);
    }
}