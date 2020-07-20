#include <cmath>


#define ARRAYSIZE(a) \
  ((sizeof(a) / sizeof(*(a))) / \
  static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))

#define M_PI 3.14159265f

#include "vk_common.h"
#include "vk_swapchain.h"
#include "vk_resources.h"
#include "vk_render.h"
#include "vk_renderprograms.h"


Image_t colorTarget = {};
Image_t depthTarget = {};

VkFramebuffer targetFramebuffer = 0;

//TODO(anton): Bad lame drawcalls
void drawCalls(VkCommandBuffer commandBuffer, Buffer_t &vb, Buffer_t &ib,
               u32 indexCount) {

}

u32 prepareFrame(Uniforms_t &ubo_vs, std::vector<glm::vec4> &pc, f64 time) {

    auto updateUBO = [&](Uniforms_t uniforms, Buffer_t &ubo_buffer, u32 width, u32 height,
                         VmaAllocator &vma_allocator) {

        uniforms.proj = glm::perspective(glm::radians(40.0f),
                                      (f32)width / (f32)height,
                                      0.1f, 256.0f);
        uniforms.proj[1][1] *= -1;

        void *data;
        vmaMapMemory(vma_allocator, ubo_buffer.vmaAlloc, &data);
        memcpy(data, &uniforms, sizeof(uniforms));
        vmaUnmapMemory(vma_allocator, ubo_buffer.vmaAlloc);
    };

    updateUBO(ubo_vs, uboBuffer, swapchain.width, swapchain.height, vma);

    auto updatePushConstants = [&](std::vector<glm::vec4> &pc, f64 t) {
        f64 scale = 0.3f;
        f64 angle = 2.0f * M_PI * t;
        pc[0].x += scale * cos(angle / 4.0f);
    };

    updatePushConstants(pc, time);

    SwapchainStatus_t swapchainStatus = updateSwapchain(swapchain, gpu.device, vk_device,
                                                        vk_context.surface, swapchainFormat,
                                                        gpu.gfxFamilyIndex);

    if (swapchainStatus == Swapchain_NotReady) {
        return U32_MAX; // surface size is zero, don't render anything this iteration.
    }

    if (swapchainStatus == Swapchain_Resized || !targetFramebuffer) {
        if (colorTarget.image) {
            destroyImage(colorTarget, vk_device, vma);
        }
        if (depthTarget.image) {
            destroyImage(depthTarget, vk_device, vma);
        }
        if (targetFramebuffer) {
            vkDestroyFramebuffer(vk_device, targetFramebuffer, nullptr);
        }

        createImage(colorTarget, vk_device, swapchain.width, swapchain.height, swapchainFormat,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
                    vma);
        createImage(depthTarget, vk_device, swapchain.width, swapchain.height, depthFormat,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, vma);
        targetFramebuffer = createFramebuffer(vk_device, renderPass, colorTarget.view, depthTarget.view, swapchain.width,
                                              swapchain.height);
    }

    u32 imageIndex = 0;
    VK_CHECK(
            vkAcquireNextImageKHR(vk_device, swapchain.swapchain, U64_MAX,
                                  acquireSemaphore, /*fence=*/VK_NULL_HANDLE, &imageIndex)
    );

    VK_CHECK(vkResetCommandPool(vk_device, commandPool, 0)); //TODO(anton): Find out if I need it and why.

    VkCommandBufferBeginInfo cmdBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo));

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

    VkClearColorValue color = {48.0f * ((float)pc[0].x * 0.1f) / 255.0f, 10.0f / 255.0f, 36.0f / 255.0f, 1};
    VkClearValue clearVals[2];
    clearVals[0].color = color;
    clearVals[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo rpBeginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rpBeginInfo.renderPass = renderPass;
    rpBeginInfo.framebuffer = targetFramebuffer;
    rpBeginInfo.renderArea.offset.x = 0;
    rpBeginInfo.renderArea.offset.y = 0;
    rpBeginInfo.renderArea.extent.width = swapchain.width;
    rpBeginInfo.renderArea.extent.height = swapchain.height;
    rpBeginInfo.clearValueCount = ARRAYSIZE(clearVals);
    rpBeginInfo.pClearValues = clearVals;

    vkCmdBeginRenderPass(commandBuffer, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdPushConstants(commandBuffer, gfxPipeLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(pc), pc.data());

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline);

    //NOTE(anton): swap the height here to account for Vulkan
    //screenspace layout? This is probably faster than multiplying
    //proj matrix by -1? -- not used right now.
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (f32) swapchain.width;
    viewport.height = (f32) swapchain.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {
            {0,                     0},
            {(u32) swapchain.width, (u32) swapchain.height}
    };

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    return imageIndex;
}

void submitFrame(u32 imageIndex) {
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
                         0, nullptr, 0, nullptr, ARRAYSIZE(copyBarriers), copyBarriers);

    VkImageCopy copyRegion = {};
    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.srcSubresource.layerCount = 1;
    copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.dstSubresource.layerCount = 1;
    copyRegion.extent = {swapchain.width, swapchain.height, 1};

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

    VK_CHECK(vkEndCommandBuffer(commandBuffer));

    VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &acquireSemaphore;
    submitInfo.pWaitDstStageMask = &waitDstStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &releaseSemaphore;

    vkQueueSubmit(queue, 1, &submitInfo, /*fence*/VK_NULL_HANDLE);

    VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &releaseSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain.swapchain;
    presentInfo.pImageIndices = &imageIndex;

    VK_CHECK(vkQueuePresentKHR(queue, &presentInfo));

    VK_CHECK(vkDeviceWaitIdle(vk_device));

}

static VkFramebuffer
createFramebuffer(VkDevice device, VkRenderPass renderPass, VkImageView colorView,
                  VkImageView depthView, u32 width, u32 height) {
    VkImageView attachments[2] = {colorView, depthView};
    VkFramebufferCreateInfo createInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    createInfo.renderPass = renderPass;
    createInfo.attachmentCount = ARRAYSIZE(attachments);
    createInfo.pAttachments = attachments;
    createInfo.width = width;
    createInfo.height = height;
    createInfo.layers = 1;

    VkFramebuffer fb = 0;
    VK_CHECK(vkCreateFramebuffer(device, &createInfo, nullptr, &fb));

    return fb;
}