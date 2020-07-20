#include "vk_resources.h"

void createBuffer(Buffer_t& result,
                  VkBufferUsageFlags usage, VmaMemoryUsage vmaUsage,
                  u32 size,
                  VmaAllocator& vma_allocator)
{
    VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    createInfo.size = size;
    createInfo.usage = usage;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo vmaCreateInfo = {};
    vmaCreateInfo.usage = vmaUsage; //NOTE(anton): If I specify usage they memory type flags will be setup automatically

    result.vmaAlloc = VK_NULL_HANDLE;
    result.vmaInfo = {};
    result.buffer = VK_NULL_HANDLE;
    VK_CHECK( vmaCreateBuffer(vma_allocator, &createInfo, &vmaCreateInfo,
                              &result.buffer,
                              &result.vmaAlloc,
                              &result.vmaInfo) );

    result.size = size;
}

void uploadBuffer(VkDevice device, VkCommandPool commandPool,
                  VkCommandBuffer commandBuffer,
                  VkQueue queue,
                  const Buffer_t& buffer, const Buffer_t& scratch,
                  const void* data, u32 size, VmaAllocator& vma_allocator)
{
    void *mappedData;
    vmaMapMemory(vma_allocator, scratch.vmaAlloc, &mappedData);
    ASSERT(scratch.size >= size);
    memcpy(mappedData, data, size);
    vmaUnmapMemory(vma_allocator, scratch.vmaAlloc);

    VK_CHECK( vkResetCommandPool(device, commandPool, 0));

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK( vkBeginCommandBuffer(commandBuffer, &beginInfo) );

    VkBufferCopy region = { 0, 0, (VkDeviceSize)size };
    vkCmdCopyBuffer(commandBuffer, scratch.buffer, buffer.buffer, 1, &region);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

    VK_CHECK(vkDeviceWaitIdle(device));
}

void createImage(Image_t& result, VkDevice device,
                 u32 width, u32 height, VkFormat format, VkImageUsageFlags usage,
                 VkImageAspectFlags aspectMask, VmaAllocator& vma_allocator)
{
    VkImageCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };

    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.format = format;
    createInfo.extent = { width, height, 1 };
    createInfo.mipLevels = 1;
    createInfo.arrayLayers = 1;
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.usage = usage;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo vmaCreateInfo = {};
    vmaCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    result.vmaAlloc = VK_NULL_HANDLE;
    result.vmaInfo = {};
    result.image = VK_NULL_HANDLE;
    VK_CHECK( vmaCreateImage(vma_allocator, &createInfo, &vmaCreateInfo,
                             &result.image,
                             &result.vmaAlloc,
                             &result.vmaInfo) );

    VkImageViewCreateInfo viewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewCreateInfo.image = result.image;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = format;
    viewCreateInfo.subresourceRange.aspectMask = aspectMask;
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = 1;
    viewCreateInfo.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(device, &viewCreateInfo, 0, &result.view));
}

void destroyImage(Image_t& image, VkDevice device, VmaAllocator& vma_allocator)
{
    vmaDestroyImage(vma_allocator, image.image, image.vmaAlloc);
}

VkImageMemoryBarrier imageMemoryBarrier(VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask)
{
    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    return barrier;
}
