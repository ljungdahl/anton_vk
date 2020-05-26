
struct Buffer
{
    VkBuffer buffer;
    VmaAllocation vmaAlloc;
    VmaAllocationInfo vmaInfo;
    size_t size;
};

struct Image
{
    VkImage image;
    VkImageView view;
    VmaAllocation vmaAlloc;
    VmaAllocationInfo vmaInfo;
};

void uploadBuffer(VkDevice device, VkCommandPool commandPool,
                  VkCommandBuffer commandBuffer,
                  VkQueue queue,
                  const Buffer& buffer, const Buffer& scratch,
                  const void* data, u32 size)
{
    ASSERT(scratch.vmaInfo.pMappedData);
    ASSERT(scratch.size >= size);
    memcpy(scratch.vmaInfo.pMappedData, data, size);

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

void createBuffer(Buffer* result, VkDevice device,
                  const VkPhysicalDeviceMemoryProperties* memProps,
                  VkBufferUsageFlags usage, VmaMemoryUsage vmaUsage,
                  VmaAllocationCreateFlags vmaFlags, u32 size,
                  VmaAllocator& vma_allocator)
{
    VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    createInfo.size = size;
    createInfo.usage = usage;

    VmaAllocationCreateInfo vmaCreateInfo = {};
    vmaCreateInfo.usage = vmaUsage;
    vmaCreateInfo.flags = vmaFlags;

    result->vmaAlloc = VK_NULL_HANDLE;
    result->vmaInfo = {};
    result->buffer = VK_NULL_HANDLE;
    VK_CHECK( vmaCreateBuffer(vma_allocator, &createInfo, &vmaCreateInfo,
                              &result->buffer,
                              &result->vmaAlloc,
                              &result->vmaInfo) );

    result->size = size;
}

void createImage(Image& result, VkDevice device,
                 u32 width, u32 height, u32 mipLevels,
                 VkFormat format, VkImageUsageFlags usage,
                 VmaAllocator& vma_allocator)
{
    VkImageCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };

    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.format = format;
    createInfo.extent = { width, height, 1 };
    createInfo.mipLevels = mipLevels;
    createInfo.arrayLayers = 1;
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.usage = usage;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImage image = 0;
    VK_CHECK(vkCreateImage(device, &createInfo, 0, &image));

    VmaAllocationCreateInfo vmaCreateInfo = {};
    vmaCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    result.vmaAlloc = VK_NULL_HANDLE;
    result.vmaInfo = {};
    result.image = VK_NULL_HANDLE;
    VK_CHECK( vmaCreateImage(vma_allocator, &createInfo, &vmaCreateInfo,
                             &result.image,
                             &result.vmaAlloc,
                             &result.vmaInfo)
              );

    VkImageAspectFlags aspectMask = (format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageViewCreateInfo viewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewCreateInfo.image = result.image;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = format;
    viewCreateInfo.subresourceRange.aspectMask = aspectMask;
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = mipLevels;
    viewCreateInfo.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(device, &viewCreateInfo, 0, &result.view));

}

void destroyImage(Image& image, VkDevice device, VmaAllocator& vma_allocator)
{
    vmaDestroyImage(vma_allocator, image.image, image.vmaAlloc);
}
