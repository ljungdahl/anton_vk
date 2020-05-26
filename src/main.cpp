#include "common.h"
#include "avk.cpp"

struct Mesh_t
{
    std::vector<Vertex_t> vertices;
    std::vector<u32> indices;
};

Mesh_t g_box;

static
VertexDescriptions_t getVertexDescriptions() {
    VertexDescriptions_t vtx_descs = {};
    
    vtx_descs.bindings.resize(1);
    vtx_descs.bindings[0].binding = 0;
    vtx_descs.bindings[0].stride = sizeof(Vertex_t);
    vtx_descs.bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    vtx_descs.attributes.resize(2);
    vtx_descs.attributes[0].binding = 0;
    vtx_descs.attributes[0].location = 0;
    vtx_descs.attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vtx_descs.attributes[0].offset = offsetof(Vertex_t, pos);

    vtx_descs.attributes[1].binding = 0;
    vtx_descs.attributes[1].location = 1;
    vtx_descs.attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vtx_descs.attributes[1].offset = offsetof(Vertex_t, normal);
    
    vtx_descs.inputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vtx_descs.inputState.vertexBindingDescriptionCount = (u32)vtx_descs.bindings.size();
    vtx_descs.inputState.pVertexBindingDescriptions = vtx_descs.bindings.data();
    vtx_descs.inputState.vertexAttributeDescriptionCount = (u32)vtx_descs.attributes.size();
    vtx_descs.inputState.pVertexAttributeDescriptions = vtx_descs.attributes.data();

    return vtx_descs;
}


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

VkDescriptorPool createDescriptorPool(VkDevice device)
{
    u32 descriptorCount = 128;
    VkDescriptorPoolSize poolSizes[] =
        {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptorCount },
        };

    VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    
    poolInfo.maxSets = descriptorCount;
    poolInfo.poolSizeCount = COUNTOF(poolSizes);
    poolInfo.pPoolSizes = poolSizes;

    VkDescriptorPool pool = 0;
    VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, 0, &pool));
    return pool;
}


VkFramebuffer createFramebuffer(VkDevice device, VkRenderPass renderPass,
                                VkImageView colorView, VkImageView depthView,
                                u32 width, u32 height)
{
    VkImageView attachments[] = { colorView, depthView };

    VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    createInfo.renderPass = renderPass;
    createInfo.attachmentCount = COUNTOF(attachments);
    createInfo.pAttachments = attachments;
    createInfo.width = width;
    createInfo.height = height;
    createInfo.layers = 1;

    VkFramebuffer framebuffer = 0;
    VK_CHECK(vkCreateFramebuffer(device, &createInfo, 0, &framebuffer));

    return framebuffer;
}



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

    VkSurfaceKHR surface;
    VK_CHECK( glfwCreateWindowSurface(instance, windowPtr, nullptr, &surface) );
        
    GPUInfo_t gpu = pickGPU(instance, surface);

    VkDevice device = createDevice(instance, &gpu);

    VmaAllocator vma = createVMAallocator(instance, gpu.device, device);
    
    VkFormat swapchainFormat = getSwapchainFormat(gpu.device, surface);
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

    VkQueue graphicsQueue = 0;
    vkGetDeviceQueue(device, gpu.graphicsFamilyIndex, 0, &graphicsQueue);

    VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VkFence waitFence = VK_NULL_HANDLE;
    VK_CHECK( vkCreateFence(device, &fenceCreateInfo, nullptr, &waitFence) );
              
    VkSemaphore acquireSemaphore = createSemaphore(device);
    VkSemaphore releaseSemaphore = createSemaphore(device);
    
    VkRenderPass renderPass = createRenderPass(device, swapchainFormat, depthFormat);

    Swapchain_t swapchain;
    createSwapchain(swapchain, gpu.device, device, surface, gpu.graphicsFamilyIndex,
                    swapchainFormat, /*oldSwapChain = */ VK_NULL_HANDLE);

    VertexDescriptions_t vtxDescs = getVertexDescriptions();
    
    bool res = false;
    Shader_t meshVS = {};
    res = loadShader(&meshVS, device, "mesh.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    ASSERT(res);
    
    Shader_t goochFS = {};
    res = loadShader(&goochFS, device, "gooch.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    ASSERT(res);

    VkPipelineCache pipelineCache = 0;

    RenderProg_t meshProg = createRenderProg(device, VK_PIPELINE_BIND_POINT_GRAPHICS);

    VkPipeline meshPipeline = createGraphicsPipeline(device, pipelineCache, renderPass,
                                                     { &meshVS, &goochFS }, meshProg.layout,
                                                     &vtxDescs);
    ASSERT(meshPipeline != VK_NULL_HANDLE);
    
    VkCommandPool commandPool = createCommandPool(device, gpu.graphicsFamilyIndex);

    VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocateInfo.commandPool = commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = 0;
    VK_CHECK( vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer) );

    VkDescriptorPool descriptorPool = createDescriptorPool(device);

    // Buffer staging_buffer = {};
    // createBuffer(&staging_buffer, device, &gpu.memProps,
    //              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    //              VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT,
    //              128 * 1024 * 1024, vma);

    // g_box.vertices.resize(4);
    // g_box.vertices[0].pos = {-1.0f, 0.0f, 0.0f};
    // g_box.vertices[1].pos = {1.0f, 0.0f, 0.0f};
    // g_box.vertices[2].pos = {0.0f, 0.0f, -1.0f};
    // g_box.vertices[3].pos = {0.0f, 0.0f, 1.0f};
    
    // Buffer box_vb = {};
    // createBuffer(&box_vb, device, &gpu.memProps,
    //              VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    //              VMA_MEMORY_USAGE_GPU_ONLY, 0,
    //              128 * 1024 * 1024, vma);

    // uploadBuffer(device, commandPool, commandBuffer, graphicsQueue,
    //              box_vb, staging_buffer, g_box.vertices.data(),
    //              g_box.vertices.size()*sizeof(g_box.vertices[0]) );

    VkClearValue clearValues[2] = {};
    clearValues[0].color = { 1.0f, 0.5f, 0.0f, 1.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkClearValue bgCV;
    bgCV.color = { 1.0f, 0.5f, 0.0f, 1.0f };
    VkClearValue depthCV;
    depthCV.depthStencil = { 1.0f, 0 };
    
    std::vector<VkClearValue> clearValsVector;
    clearValsVector.push_back(bgCV);
    clearValsVector.push_back(depthCV);

    
    Image colorTarget = {};
    Image depthTarget = {};
    VkFramebuffer targetFB = VK_NULL_HANDLE;

    u32 frameCtr = 0;
    while(!glfwWindowShouldClose(windowPtr))
        //while(frameCtr < 2)
    {        
        glfwPollEvents();

        vkWaitForFences(device, 1, &waitFence, VK_TRUE, U64_MAX);
        vkResetFences(device, 1, &waitFence);

        SwapchainStatus swapchainStatus = updateSwapchain(swapchain, gpu.device, device,
                                                          surface, gpu.graphicsFamilyIndex,
                                                          swapchainFormat);

        if (swapchainStatus == Swapchain_NotReady)
        {
            continue;
        }
        
        if (swapchainStatus == Swapchain_Resized || !targetFB)
        {
            if (colorTarget.image)
                destroyImage(colorTarget, device, vma);
            if (depthTarget.image)
                destroyImage(depthTarget, device, vma);
            if (targetFB)
                vkDestroyFramebuffer(device, targetFB, 0);

            createImage(colorTarget, device, swapchain.width, swapchain.height,
                        1, swapchainFormat,
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                    vma);
            
            createImage(depthTarget, device, swapchain.width, swapchain.height,
                        1, depthFormat,
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        vma);
            
            targetFB = createFramebuffer(device, renderPass, colorTarget.view,
                                         depthTarget.view, swapchain.width, swapchain.height);
            
        }

        uint32_t imageIndex = 0;
        VK_CHECK(vkAcquireNextImageKHR(device, swapchain.swapchain,
                                       U64_MAX, acquireSemaphore,
                                       VK_NULL_HANDLE, &imageIndex));

        VK_CHECK(vkResetCommandPool(device, commandPool, 0));

        VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK( vkBeginCommandBuffer(commandBuffer, &beginInfo) );

        VkImageSubresourceRange imageSubresourceRange = {
            VK_IMAGE_ASPECT_COLOR_BIT,                    // VkImageAspectFlags                     aspectMask
            0,                                            // uint32_t                               baseMipLevel
            1,                                            // uint32_t                               levelCount
            0,                                            // uint32_t                               baseArrayLayer
            1                                             // uint32_t                               layerCount
        };

        VkImageMemoryBarrier presentToDrawBarrier= {
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,     // VkStructureType                        sType
            nullptr,                                    // const void                            *pNext
            VK_ACCESS_MEMORY_READ_BIT,                  // VkAccessFlags                          srcAccessMask
            VK_ACCESS_MEMORY_READ_BIT,               // VkAccessFlags                          dstAccessMask
            VK_IMAGE_LAYOUT_UNDEFINED,                  // VkImageLayout                          oldLayout
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,       // VkImageLayout                          newLayout
            VK_QUEUE_FAMILY_IGNORED,                    // uint32_t                               srcQueueFamilyIndex
            VK_QUEUE_FAMILY_IGNORED,                    // uint32_t                               dstQueueFamilyIndex
            colorTarget.image,                       // VkImage                                image
            imageSubresourceRange,                     // VkImageSubresourceRange                subresourceRange
        };

    
        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             0, 0, nullptr, 0, nullptr, 1,
                             &presentToDrawBarrier);


        // BEGIN RENDER PASS
        VkRenderPassBeginInfo passBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        passBeginInfo.renderPass = renderPass;
        passBeginInfo.framebuffer = targetFB;
        passBeginInfo.renderArea.extent.width = swapchain.width;
        passBeginInfo.renderArea.extent.height = swapchain.height;
        passBeginInfo.clearValueCount = (u32)clearValsVector.size();
        passBeginInfo.pClearValues = clearValsVector.data();
        
        vkCmdBeginRenderPass(commandBuffer, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline);
        
        VkViewport viewport = { 0, (f32)swapchain.height, (f32)swapchain.width, (f32)swapchain.height, 0, 1 };
        VkRect2D scissor = { {0, 0}, {(u32)swapchain.width, (u32)swapchain.height} };
        
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

                
        vkCmdEndRenderPass(commandBuffer);

        vkCmdClearColorImage(commandBuffer, colorTarget.image, VK_IMAGE_LAYOUT_GENERAL,
                             &clearValsVector[0].color, 1, &imageSubresourceRange);

        VkImageMemoryBarrier barrier_from_draw_to_present = {
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,           // VkStructureType                        sType
            nullptr,                                          // const void                            *pNext
            VK_ACCESS_MEMORY_READ_BIT,                        // VkAccessFlags                          srcAccessMask
            VK_ACCESS_MEMORY_READ_BIT,                        // VkAccessFlags                          dstAccessMask
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                  // VkImageLayout                          oldLayout
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                  // VkImageLayout                          newLayout
            VK_QUEUE_FAMILY_IGNORED,                    // uint32_t                               srcQueueFamilyIndex
            VK_QUEUE_FAMILY_IGNORED,                    // uint32_t                               dstQueueFamilyIndex
            colorTarget.image,                          // VkImage                                image
            imageSubresourceRange                           // VkImageSubresourceRange                subresourceRange
        };
        
        vkCmdPipelineBarrier( commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                              VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                              0, 0, nullptr, 0, nullptr, 1, &barrier_from_draw_to_present );

    
        VK_CHECK(vkEndCommandBuffer(commandBuffer));

        // PRESENT
        VkPipelineStageFlags submitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &acquireSemaphore;
        submitInfo.pWaitDstStageMask = &submitStageMask;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &releaseSemaphore;

        VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, waitFence));

        VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &releaseSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain.swapchain;
        presentInfo.pImageIndices = &imageIndex;

        VK_CHECK( vkQueuePresentKHR(graphicsQueue, &presentInfo) );

        VkResult wfi = vkDeviceWaitIdle(device);
        
        ////////////////////////////////////////////////////////////////
        char title[256];
        sprintf(title, "anton_vk - frame %i - imageIndex %i", frameCtr, imageIndex);
        glfwSetWindowTitle(windowPtr, title);
        frameCtr++;
        
    }

    
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    
    if(windowPtr)
    {
        glfwDestroyWindow(windowPtr);
    }
    glfwTerminate();
    
    return 0;   
}
