#pragma once

#include "vk_common.h"

static VkFramebuffer
createFramebuffer(VkDevice device, VkRenderPass renderPass, VkImageView colorView,
                  VkImageView depthView, u32 width, u32 height);

u32 prepareFrame();
void drawMesh(u32 startVertex, u32 startIndex, u32 indexCount, f64 time);
void submitFrame(u32 imageIndex);

void updateUniforms();

