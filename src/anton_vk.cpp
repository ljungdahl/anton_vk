
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
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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
    
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    
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
    
    VkFormat swapchainFormat = getSwapchainFormat(gpu.device, surface);
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

    VkQueue queue = 0;
    vkGetDeviceQueue(device, gpu.gfxFamilyIndex, 0, &queue);
              
    VkSemaphore acquireSemaphore = createSemaphore(device);
    VkSemaphore releaseSemaphore = createSemaphore(device);
    // VkFence waitFence = createWaitFence(device); // NOTE(anton): is this even needed?

    Swapchain_t swapchain = {};
    createSwapchain(swapchain, gpu.device, device, surface, swapchainFormat, gpu.gfxFamilyIndex, /*oldSwapchain=*/VK_NULL_HANDLE);

    VkRenderPass renderPass = createRenderPass(device, swapchainFormat);

    VkCommandPool commandPool = createCommandPool(device, gpu.gfxFamilyIndex);
    VkCommandBuffer commandBuffer = 0;
    allocateCommandBuffer(device, commandPool, &commandBuffer);

    Image colorTarget = {};
    VkFramebuffer targetFramebuffer = 0;
    
    u32 frameCounter = 0;
    while(!glfwWindowShouldClose(windowPtr))
    {
        glfwPollEvents();

        processKeyInput(windowPtr);


        // End input processing - begin rendering calls
        
        SwapchainStatus_t swapchainStatus = updateSwapchain(swapchain, gpu.device, device, surface, swapchainFormat, gpu.gfxFamilyIndex);

        if (swapchainStatus == Swapchain_NotReady)
        {
            continue; // surface size is zero, don't render anything this iteration.
        }

        if (swapchainStatus == Swapchain_Resized || !targetFramebuffer)
        {
            if(colorTarget.image)
            {
                destroyImage(colorTarget, device, vma);
            }
            if(targetFramebuffer)
            {
                vkDestroyFramebuffer(device, targetFramebuffer, nullptr);
            }
            
            createImage(colorTarget, device, swapchain.width, swapchain.height, swapchainFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, vma);
            targetFramebuffer = createFramebuffer(device, renderPass, colorTarget.view, swapchain.width, swapchain.height);

        }

        u32 imageIndex = 0;
        VK_CHECK( vkAcquireNextImageKHR(device, swapchain.swapchain, U64_MAX, acquireSemaphore, /*fence=*/VK_NULL_HANDLE, &imageIndex) ); 
        
        VK_CHECK( vkResetCommandPool(device, commandPool, 0) ); //TODO(anton): Find out if I need it and why.

        VkCommandBufferBeginInfo cmdBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        
        VK_CHECK( vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo) );

        VkImageMemoryBarrier renderBeginBarrier = imageMemoryBarrier(colorTarget.image,
                                                                     0,
                                                                     0,
                                                                     VK_IMAGE_LAYOUT_UNDEFINED,
                                                                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_DEPENDENCY_BY_REGION_BIT,
                             0, 0, 0, 0, 1, &renderBeginBarrier);
        
        VkClearColorValue color = { 48.0f/255.0f, 10.0f/255.0f, 36.0f/255.0f, 1 };
        VkClearValue clearVals[2];
        clearVals[0].color = color;
        clearVals[1].depthStencil = {1.0f, 0};
        
        VkRenderPassBeginInfo rpBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        rpBeginInfo.renderPass = renderPass;
        rpBeginInfo.framebuffer = targetFramebuffer;
        rpBeginInfo.renderArea.offset.x = 0;
        rpBeginInfo.renderArea.offset.y = 0;
        rpBeginInfo.renderArea.extent.width = swapchain.width;
        rpBeginInfo.renderArea.extent.height = swapchain.height;
        rpBeginInfo.clearValueCount = ARRAYSIZE(clearVals);
        rpBeginInfo.pClearValues = clearVals;

        vkCmdBeginRenderPass(commandBuffer, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = { 0, (f32)swapchain.height, (f32)swapchain.width, -(f32)swapchain.height, 0, 1 }; //NOTE(anton): swap the height here to account for Vulkan screenspace layout? This is probably faster than multiplying proj matrix by -1?
        VkRect2D scissor = { {0, 0}, {(u32)swapchain.width, (u32)swapchain.height} };

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdEndRenderPass(commandBuffer);

        VkImageMemoryBarrier copyBarriers[2] =
            {
                imageMemoryBarrier(colorTarget.image,
                                   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                   VK_ACCESS_TRANSFER_READ_BIT,
                                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),
                
                imageMemoryBarrier(swapchain.images[imageIndex],
                                   0,
                                   VK_ACCESS_TRANSFER_WRITE_BIT,
                                   VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            };
        
        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_DEPENDENCY_BY_REGION_BIT,
                             0, 0, 0, 0, ARRAYSIZE(copyBarriers), copyBarriers);

        VkImageCopy copyRegion = {};
        copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.srcSubresource.layerCount = 1;
        copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.dstSubresource.layerCount = 1;
        copyRegion.extent = { swapchain.width, swapchain.height, 1 };

        vkCmdCopyImage(commandBuffer,
                       colorTarget.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       swapchain.images[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &copyRegion);

        VkImageMemoryBarrier presentBarrier = imageMemoryBarrier(swapchain.images[imageIndex],
                                                                 VK_ACCESS_TRANSFER_WRITE_BIT,
                                                                 0,
                                                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                                 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_DEPENDENCY_BY_REGION_BIT,
                             0, 0, 0, 0, 1, &presentBarrier);

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

        // End render calls

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





// --------------------------------------------------------------
/* NOTE(anton): The render to target -> copy to swapchain -> present setup explained.
   Swapchain is created with .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
   I have a variable number of swapchain images (minimum 2).

   I only have one color target image & imageview. I only have one
   target framebuffer (linked to this color target image view).

   Initial renderpass color attachment layout is undefined, and the
   final is VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL.

   Before beginning renderpass I transition colorTarget.image to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL.
   This uses the renderBeginBarrier and pipeline barrier.
   Then in the renderpass I can render the clear value / or whatever to the target.

   Then I need to change the layout again so I can copy. Since
   swapchain images are initialised to
   VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, I have to change the layout of the
   swapchain images as well. This is done in the copyBarriers[2] construct.
   color target is changed as:
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL -> VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL.
   swapchain image is changed as:
      VK_IMAGE_LAYOUT_UNDEFINED -> VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL. (we know it's PRESENT_SRC, but UNDEFINED just mean we don't care).

   Then I can copy with vkCmdCopyImage.

   Then we need the last barrier to change the swapchain image as:
       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL -> VK_IMAGE_LAYOUT_PRESENT_SRC_KHR.
   Here we use presentBarrier and corresponding vkCmdPipelineBarrier.

   Now we can end the command buffer and submit it to queue. THe above operations will then be executed and
   the result from the queue can be presented.

   ---

   So why are we doing this? I guess it's because I don't need more than one color target and one framebuffer?
   Other methods have been using one framebuffer for each swapchain image etc. Need to watch more zeux niagara streams
   to find out.
 */
