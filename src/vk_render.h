#pragma once

#include "vk_common.h"

static VkFramebuffer
createFramebuffer(VkDevice device, VkRenderPass renderPass, VkImageView colorView,
                  VkImageView depthView, u32 width, u32 height);

u32 prepareFrame(Uniforms_t &uboVS, std::vector<glm::vec4> &pc, f64 time);

void submitFrame(u32 imageIndex);

void drawCalls(VkCommandBuffer commandBuffer, Buffer_t &vb, Buffer_t &ib, u32 indexCount);