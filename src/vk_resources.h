#pragma once

#include "vk_common.h"

VkImageMemoryBarrier imageMemoryBarrier(VkImage image, VkAccessFlags srcAccessMask,
                                        VkAccessFlags dstAccessMask,
                                        VkImageLayout oldLayout, VkImageLayout newLayout,
                                        VkImageAspectFlags aspectMask);

void destroyImage(Image_t &image, VkDevice device, VmaAllocator &vma_allocator);

void createImage(Image_t &result, VkDevice device,
                 u32 width, u32 height, VkFormat format, VkImageUsageFlags usage,
                 VkImageAspectFlags aspectMask, VmaAllocator &vma_allocator);

void uploadBuffer(VkDevice device, VkCommandPool commandPool,
                  VkCommandBuffer commandBuffer,
                  VkQueue queue,
                  const Buffer_t &buffer, const Buffer_t &scratch,
                  const void *data, u32 size, VmaAllocator &vma_allocator);

void createBuffer(Buffer_t &result,
                  VkBufferUsageFlags usage, VmaMemoryUsage vmaUsage,
                  u32 size,
                  VmaAllocator &vma_allocator);