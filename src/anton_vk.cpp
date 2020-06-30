#include <cmath>
#define M_PI 3.14159265f

#include "common.h"
#include "device.cpp"
#include "swapchain.cpp"
#include "resources.cpp"
#include "shaders.cpp"

// --------------------------------------------------------------
// Forward declare
void processKeyInput(GLFWwindow* windowPtr);

// --------------------------------------------------------------
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

VkRenderPass createRenderPass(VkDevice device, VkFormat colorFormat, VkFormat depthFormat)
{

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

    attachments[1].format = depthFormat;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

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

VkFramebuffer createFramebuffer(VkDevice device, VkRenderPass renderPass, VkImageView colorView, VkImageView depthView, u32 width, u32 height)
{
    VkImageView attachments[2] = { colorView, depthView };
    VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    createInfo.renderPass = renderPass;
    createInfo.attachmentCount = ARRAYSIZE(attachments);
    createInfo.pAttachments = attachments;
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
    //VkFormat depthFormat = getDepthFormat(gpu.device); //TODO(anton): fix so everything works with general depth format and stencil!
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

    VkQueue queue = 0;
    vkGetDeviceQueue(device, gpu.gfxFamilyIndex, 0, &queue);
              
    VkSemaphore acquireSemaphore = createSemaphore(device);
    VkSemaphore releaseSemaphore = createSemaphore(device);

    Swapchain_t swapchain = {};
    createSwapchain(swapchain, gpu.device, device, surface, swapchainFormat, gpu.gfxFamilyIndex, /*oldSwapchain=*/VK_NULL_HANDLE);

    VkRenderPass renderPass = createRenderPass(device, swapchainFormat, depthFormat);

    VkCommandPool commandPool = createCommandPool(device, gpu.gfxFamilyIndex);
    VkCommandBuffer commandBuffer = 0;
    allocateCommandBuffer(device, commandPool, &commandBuffer);

    MVPmatrices_t uboVS;

    VertexDescriptions_t vtxDescs = getVertexDescriptions();
    
    Mesh_t cubeMesh;
    loadObj("../assets/coords2_soft.obj", &cubeMesh);
    
    bool res = false;
    Shader_t meshVS = {};
    res = loadShader(meshVS, device, "mesh.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    ASSERT(res);
    
    Shader_t goochFS = {};
    Shader_t lambertFS = {};
    res = loadShader(goochFS, device, "gooch.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    ASSERT(res);
    res = loadShader(lambertFS, device, "lambert.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    ASSERT(res);
    
    Buffer_t vb = {};
    Buffer_t vb_staging = {};
    u32 vbSize = (u32)cubeMesh.vertices.size()*sizeof(cubeMesh.vertices[0]);
    createBuffer(vb,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VMA_MEMORY_USAGE_GPU_ONLY,
                 vbSize, vma);
    createBuffer(vb_staging,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VMA_MEMORY_USAGE_CPU_ONLY,
                 vbSize, vma);
    uploadBuffer(device, commandPool, commandBuffer, queue,
                 vb, vb_staging,
                 cubeMesh.vertices.data(), vbSize, vma);
    
    Buffer_t ib = {};
    Buffer_t ib_staging = {};
    u32 ibSize = (u32)cubeMesh.indices.size()*sizeof(cubeMesh.indices[0]);
    createBuffer(ib,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VMA_MEMORY_USAGE_GPU_ONLY,
                 ibSize, vma);
    createBuffer(ib_staging,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VMA_MEMORY_USAGE_CPU_ONLY,
                 ibSize, vma);
    uploadBuffer(device, commandPool, commandBuffer, queue,
                 ib, ib_staging,
                 cubeMesh.indices.data(), ibSize, vma);
    
    Buffer_t uboBuffer = {};
    createBuffer(uboBuffer,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VMA_MEMORY_USAGE_CPU_TO_GPU,
                 sizeof(uboVS), vma);

    glm::vec3 initCameraPos = glm::vec3(4.0f, 3.0f, 7.0f);
    glm::mat4 initView = glm::lookAt(initCameraPos, // esye
                                     glm::vec3(0.0f, 0.0f, 0.0f), // center
                                     glm::vec3(0.0f, 1.0f, 0.0f)); // up 

    uboVS.model = glm::mat4(1.0f);
    auto rotMat = glm::rotate(uboVS.model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    //auto transMat = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.02f, -1.0f));
    auto transMat = glm::mat4(1.0f);
    uboVS.model *= rotMat;
    uboVS.model *= transMat;
    uboVS.view = initView;

    PushConstants_t pushConstants;
    glm::vec3 initLightPos = glm::vec3(30.0f, 0.0f, 0.0f);
    //glm::vec3 initLightPos = initCameraPos;
    pushConstants.lightPos = initLightPos;
    
    VkDescriptorPool descPool = createDescriptorPool(device, 1);
    VkDescriptorSetLayout descSetLayout;
    descSetLayout = createDescriptorSetLayout(device);
    VkDescriptorSet descSets[1];
    allocateDescriptorSet(device, descPool, descSetLayout, descSets, 1);
    updateDescriptorSet(device, uboBuffer, /*offset*/0, /*range*/sizeof(uboVS), descSets);

    VkPipelineCache pipelineCache = 0;
    VkPipelineLayout gfxPipeLayout = 0;

    VkPushConstantRange pushConstantRange;
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.size = sizeof(pushConstants);
    pushConstantRange.offset = 0;
    
    VkPipelineLayoutCreateInfo layout_create_info = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    layout_create_info.setLayoutCount = 1;
    layout_create_info.pSetLayouts = &descSetLayout;
    layout_create_info.pushConstantRangeCount = 1;
    layout_create_info.pPushConstantRanges = &pushConstantRange;
    
    VK_CHECK( vkCreatePipelineLayout(device, &layout_create_info, nullptr, &gfxPipeLayout) );
    VkPipeline meshPipeline = createGraphicsPipeline(device, pipelineCache, renderPass,
                                                     meshVS, lambertFS, gfxPipeLayout,
                                                     &vtxDescs);
    
    Image_t colorTarget = {};
    Image_t depthTarget = {};
    VkFramebuffer targetFramebuffer = 0;
    
    u32 frameCounter = 0;

    f64 deltaTime = 0.0f;
    f64 elapsedTime = 0.0f;
    f64 previousTime = 0.0f;
    glfwSetTime(elapsedTime);
    
    while(!glfwWindowShouldClose(windowPtr))
    {
        glfwPollEvents();

        deltaTime = glfwGetTime()-previousTime;
        
        processKeyInput(windowPtr);

        // End input processing - begin rendering calls
        
        SwapchainStatus_t swapchainStatus = updateSwapchain(swapchain, gpu.device, device, surface, swapchainFormat, gpu.gfxFamilyIndex);

        auto updateUBO = [&](MVPmatrices_t& uboVS, Buffer_t& uboBuffer, u32 width, u32 height, VmaAllocator& vma_allocator)
                         {
                             uboVS.proj = glm::perspective(glm::radians(40.0f),
                                                           (f32)width / (f32)height,
                                                           0.1f, 256.0f);
                             uboVS.proj[1][1] *= -1;
                             
                             //auto rotMat = glm::rotate(glm::mat4(1.0f), 0.003f, glm::vec3(0.0f, 1.0f, 0.0f));
                             //auto transMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
                             //uboVS.model *= rotMat;
                             //uboVS.model *= transMat;
            
                             void *data;
                             vmaMapMemory(vma_allocator, uboBuffer.vmaAlloc, &data);
                             memcpy(data, &uboVS, sizeof(uboVS));
                             vmaUnmapMemory(vma_allocator, uboBuffer.vmaAlloc);
                         };

        auto updatePushConstants = [&](PushConstants_t &pc, f64 t)
                                   {
                                       f64 scale = 100.0f;
                                       pc.lightPos.z += scale*t;
                                   };
        
        updateUBO(uboVS, uboBuffer, swapchain.width, swapchain.height, vma);
        //updatePushConstants(pushConstants, deltaTime);
        
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
            if(depthTarget.image)
            {
                destroyImage(depthTarget, device, vma);
            }
            if(targetFramebuffer)
            {
                vkDestroyFramebuffer(device, targetFramebuffer, nullptr);
            }
            
            createImage(colorTarget, device, swapchain.width, swapchain.height, swapchainFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT, vma);
            createImage(depthTarget, device, swapchain.width, swapchain.height, depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, vma);
            targetFramebuffer = createFramebuffer(device, renderPass, colorTarget.view, depthTarget.view, swapchain.width, swapchain.height);
        }

        
        
        u32 imageIndex = 0;
        VK_CHECK( vkAcquireNextImageKHR(device, swapchain.swapchain, U64_MAX, acquireSemaphore, /*fence=*/VK_NULL_HANDLE, &imageIndex) ); 
        
        VK_CHECK( vkResetCommandPool(device, commandPool, 0) ); //TODO(anton): Find out if I need it and why.

        VkCommandBufferBeginInfo cmdBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        
        VK_CHECK( vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo) );

        VkImageMemoryBarrier renderBeginBarriers[2] =
            {
                imageMemoryBarrier(colorTarget.image,
                                   0,
                                   0,
                                   VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                   VK_IMAGE_ASPECT_COLOR_BIT),
                
                imageMemoryBarrier(depthTarget.image,
                                   0,
                                   0,
                                   VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                   VK_IMAGE_ASPECT_DEPTH_BIT)
            };

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_DEPENDENCY_BY_REGION_BIT,
                             0, 0, 0, 0, ARRAYSIZE(renderBeginBarriers), renderBeginBarriers);
        
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

        vkCmdPushConstants(commandBuffer, gfxPipeLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);
        
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline);

        //NOTE(anton): swap the height here to account for Vulkan
        //screenspace layout? This is probably faster than multiplying
        //proj matrix by -1? -- not used right now.
        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (f32)swapchain.width;
        viewport.height = (f32)swapchain.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        
        VkRect2D scissor = { {0, 0}, {(u32)swapchain.width, (u32)swapchain.height} };

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkDeviceSize offset = 0; //TODO(anton): Wat does this even do
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vb.buffer, &offset);
        vkCmdBindIndexBuffer(commandBuffer, ib.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                gfxPipeLayout, 0, 1, descSets, 0, nullptr);
        
        vkCmdDrawIndexed(commandBuffer, cubeMesh.indexCount, 1, 0, 0, 0);
        //vkCmdDraw(commandBuffer, (u32)cubeMesh.vertices.size(), 1, 0, 0);
        vkCmdEndRenderPass(commandBuffer);

        VkImageMemoryBarrier copyBarriers[2] =
            {
                imageMemoryBarrier(colorTarget.image,
                                   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                   VK_ACCESS_TRANSFER_READ_BIT,
                                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                   VK_IMAGE_ASPECT_COLOR_BIT),
                
                imageMemoryBarrier(swapchain.images[imageIndex],
                                   0,
                                   VK_ACCESS_TRANSFER_WRITE_BIT,
                                   VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   VK_IMAGE_ASPECT_COLOR_BIT)
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
                                                                 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                                 VK_IMAGE_ASPECT_COLOR_BIT);
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
        previousTime = elapsedTime;
        elapsedTime = glfwGetTime();
        frameCounter++;
        char title[256];
        sprintf(title, "frame: %i - imageIndex: %i - delta time: %f - elapsed time: %f", frameCounter, imageIndex, deltaTime, elapsedTime);
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


// static // This is note used?
// VkFence createWaitFence(VkDevice device)
// {    
//     VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
//     fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
//     VkFence waitFence = VK_NULL_HANDLE;
//     VK_CHECK( vkCreateFence(device, &fenceCreateInfo, nullptr, &waitFence) );
//     return waitFence;
// }
