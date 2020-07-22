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

Image_t vk_colorTarget = {};
Image_t vk_depthTarget = {};

VkFramebuffer vk_targetFramebuffer = 0;

void updateUniforms() {

    auto updateUBO = [&](Uniforms_t uniforms, Buffer_t &ubo_buffer, u32 width, u32 height,
                         VmaAllocator &vma_allocator) {

        uniforms.proj = glm::perspective(glm::radians(40.0f),
                                         (f32) width / (f32) height,
                                        0.1f, 256.0f);
        uniforms.proj[1][1] *= -1;

        void *data;
        vmaMapMemory(vma_allocator, ubo_buffer.vmaAlloc, &data);
        memcpy(data, &uniforms, sizeof(uniforms));
        vmaUnmapMemory(vma_allocator, ubo_buffer.vmaAlloc);
    };

    updateUBO(vk_uniformData, vk_uniformBuffer, vk_swapchain.width, vk_swapchain.height, vk_vma);
}

u32 prepareFrame() {

    SwapchainStatus_t swapchainStatus = updateSwapchain(vk_swapchain, vk_gpu.device, vk_device,
                                                        vk_context.surface, vk_swapchainFormat,
                                                        vk_gpu.gfxFamilyIndex);

    if (swapchainStatus == Swapchain_NotReady) {
        return U32_MAX; // surface size is zero, don't render anything this iteration.
    }

    if (swapchainStatus == Swapchain_Resized || !vk_targetFramebuffer) {
        if (vk_colorTarget.image) {
            destroyImage(vk_colorTarget, vk_device, vk_vma);
        }
        if (vk_depthTarget.image) {
            destroyImage(vk_depthTarget, vk_device, vk_vma);
        }
        if (vk_targetFramebuffer) {
            vkDestroyFramebuffer(vk_device, vk_targetFramebuffer, nullptr);
        }

        createImage(vk_colorTarget, vk_device, vk_swapchain.width, vk_swapchain.height, vk_swapchainFormat,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
                    vk_vma);
        createImage(vk_depthTarget, vk_device, vk_swapchain.width, vk_swapchain.height, vk_depthFormat,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, vk_vma);
        vk_targetFramebuffer = createFramebuffer(vk_device, vk_renderPass, vk_colorTarget.view, vk_depthTarget.view,
                                                 vk_swapchain.width,
                                                 vk_swapchain.height);
    }

    u32 imageIndex = 0;
    VK_CHECK(
            vkAcquireNextImageKHR(vk_device, vk_swapchain.swapchain, U64_MAX,
                                  vk_acquireSemaphore, /*fence=*/VK_NULL_HANDLE, &imageIndex)
    );

    VK_CHECK(vkResetCommandPool(vk_device, vk_commandPool, 0)); //TODO(anton): Find out if I need it and why.

    VkCommandBufferBeginInfo cmdBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(vk_commandBuffer, &cmdBeginInfo));

    VkImageMemoryBarrier renderBeginBarriers[2] =
            {
                    imageMemoryBarrier(vk_colorTarget.image,
                                       0,
                                       0,
                                       VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                       VK_IMAGE_ASPECT_COLOR_BIT),

                    imageMemoryBarrier(vk_depthTarget.image,
                                       0,
                                       0,
                                       VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                       VK_IMAGE_ASPECT_DEPTH_BIT)
            };

    vkCmdPipelineBarrier(vk_commandBuffer,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_DEPENDENCY_BY_REGION_BIT,
                         0, 0, 0, 0, ARRAYSIZE(renderBeginBarriers), renderBeginBarriers);

    VkClearColorValue color = {48.0f / 255.0f, 10.0f / 255.0f, 36.0f / 255.0f, 1};
    VkClearValue clearVals[2];
    clearVals[0].color = color;
    clearVals[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo rpBeginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rpBeginInfo.renderPass = vk_renderPass;
    rpBeginInfo.framebuffer = vk_targetFramebuffer;
    rpBeginInfo.renderArea.offset.x = 0;
    rpBeginInfo.renderArea.offset.y = 0;
    rpBeginInfo.renderArea.extent.width = vk_swapchain.width;
    rpBeginInfo.renderArea.extent.height = vk_swapchain.height;
    rpBeginInfo.clearValueCount = ARRAYSIZE(clearVals);
    rpBeginInfo.pClearValues = clearVals;

    vkCmdBeginRenderPass(vk_commandBuffer, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);


    //NOTE(anton): swap the height here to account for Vulkan
    //screenspace layout? This is probably faster than multiplying
    //proj matrix by -1? -- not used right now.
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (f32) vk_swapchain.width;
    viewport.height = (f32) vk_swapchain.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {
            {0,                        0},
            {(u32) vk_swapchain.width, (u32) vk_swapchain.height}
    };

    vkCmdSetViewport(vk_commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(vk_commandBuffer, 0, 1, &scissor);

    // Bind the whole buffers and then we acces using the vkCmdDrawIndexed command?
    // ie offsets are zero here.
    VkDeviceSize vtxOffset = 0;
    VkDeviceSize idxOffset = 0;
    vkCmdBindVertexBuffers(vk_commandBuffer, 0, 1, &vk_staticVertexBuffer.buffer, &vtxOffset);
    vkCmdBindIndexBuffer(vk_commandBuffer, vk_staticIndexBuffer.buffer, idxOffset, VK_INDEX_TYPE_UINT32);

    return imageIndex;
}

void drawMesh(u32 startVertex, u32 startIndex, u32 indexCount, f64 time) {
//
//    Logger::Trace("startVertex %i, startIndex %i, indexCount %i",
//            startVertex, startIndex, indexCount);
    auto updateLightsPushConstants = [&](glm::vec4 &lights, f64 t) {
        f64 scale = 0.3f;
        f64 angle = 2.0f * M_PI * t;
        lights.x += scale * cos(angle / 4.0f);
    };

    updateLightsPushConstants(vk_pushConstants.lights[0], time);

    vkCmdPushConstants(vk_commandBuffer, vk_gfxPipeLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(vk_pushConstants), &vk_pushConstants);

    vkCmdBindPipeline(vk_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_meshPipeline);

    vkCmdBindDescriptorSets(vk_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_gfxPipeLayout,
                            0, 1, vk_descSets, 0, nullptr);

    vkCmdDrawIndexed(vk_commandBuffer, indexCount, 1, startIndex, startVertex, 0);
}

void submitFrame(u32 imageIndex) {
    vkCmdEndRenderPass(vk_commandBuffer);

    VkImageMemoryBarrier copyBarriers[2] =
            {
                    imageMemoryBarrier(vk_colorTarget.image,
                                       VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                       VK_ACCESS_TRANSFER_READ_BIT,
                                       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                       VK_IMAGE_ASPECT_COLOR_BIT),

                    imageMemoryBarrier(vk_swapchain.images[imageIndex],
                                       0,
                                       VK_ACCESS_TRANSFER_WRITE_BIT,
                                       VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                       VK_IMAGE_ASPECT_COLOR_BIT)
            };

    vkCmdPipelineBarrier(vk_commandBuffer,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_DEPENDENCY_BY_REGION_BIT,
                         0, nullptr, 0, nullptr, ARRAYSIZE(copyBarriers), copyBarriers);

    VkImageCopy copyRegion = {};
    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.srcSubresource.layerCount = 1;
    copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.dstSubresource.layerCount = 1;
    copyRegion.extent = {vk_swapchain.width, vk_swapchain.height, 1};

    vkCmdCopyImage(vk_commandBuffer,
                   vk_colorTarget.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   vk_swapchain.images[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &copyRegion);

    VkImageMemoryBarrier presentBarrier = imageMemoryBarrier(vk_swapchain.images[imageIndex],
                                                             VK_ACCESS_TRANSFER_WRITE_BIT,
                                                             0,
                                                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                             VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                             VK_IMAGE_ASPECT_COLOR_BIT);

    vkCmdPipelineBarrier(vk_commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_DEPENDENCY_BY_REGION_BIT,
                         0, 0, 0, 0, 1, &presentBarrier);

    VK_CHECK(vkEndCommandBuffer(vk_commandBuffer));

    VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &vk_acquireSemaphore;
    submitInfo.pWaitDstStageMask = &waitDstStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vk_commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &vk_releaseSemaphore;

    vkQueueSubmit(vk_queue, 1, &submitInfo, /*fence*/VK_NULL_HANDLE);

    VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &vk_releaseSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &vk_swapchain.swapchain;
    presentInfo.pImageIndices = &imageIndex;

    VK_CHECK(vkQueuePresentKHR(vk_queue, &presentInfo));

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