struct Image
{
    VkImage image;
    VkImageView view;
    VmaAllocation vmaAlloc;
    VmaAllocationInfo vmaInfo;
};

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format)
{
    VkImageAspectFlags aspectMask = (format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    createInfo.image    = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format   = format;
    createInfo.subresourceRange.aspectMask      = aspectMask;
    //createInfo.subresourceRange.baseMipLevel    = 0;
    createInfo.subresourceRange.levelCount      = 1;
    //createInfo.subresourceRange.baseArrayLayer  = 0;
    createInfo.subresourceRange.layerCount      = 1;

    VkImageView view = 0;
    VK_CHECK(vkCreateImageView(device, &createInfo, 0, &view));

    return view;
}

// void createImage(Image& result, VkDevice device,
//                  u32 width, u32 height, VkFormat format, VkImageUsageFlags usage,
//                  VmaAllocator& vma_allocator)
// {
//     VkImageCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };

//     createInfo.imageType = VK_IMAGE_TYPE_2D;
//     createInfo.format = format;
//     createInfo.extent = { width, height, 1 };
//     //createInfo.mipLevels = 1;
//     createInfo.arrayLayers = 1;
//     createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
//     createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
//     createInfo.usage = usage;
//     createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

//     VkImage image = 0;
//     VK_CHECK(vkCreateImage(device, &createInfo, 0, &image));

//     VmaAllocationCreateInfo vmaCreateInfo = {};
//     vmaCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
//     vmaCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

//     result.vmaAlloc = VK_NULL_HANDLE;
//     result.vmaInfo = {};
//     result.image = VK_NULL_HANDLE;
//     VK_CHECK( vmaCreateImage(vma_allocator, &createInfo, &vmaCreateInfo,
//                              &result.image,
//                              &result.vmaAlloc,
//                              &result.vmaInfo) );

//     VkImageAspectFlags aspectMask = (format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

//     VkImageViewCreateInfo viewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
//     viewCreateInfo.image = result.image;
//     viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
//     viewCreateInfo.format = format;
//     viewCreateInfo.subresourceRange.aspectMask = aspectMask;
//     //viewCreateInfo.subresourceRange.baseMipLevel = 0;
//     viewCreateInfo.subresourceRange.levelCount = 1;
//     viewCreateInfo.subresourceRange.layerCount = 1;

//     VK_CHECK(vkCreateImageView(device, &viewCreateInfo, 0, &result.view));

// }

// void destroyImage(Image& image, VkDevice device, VmaAllocator& vma_allocator)
// {
//     vmaDestroyImage(vma_allocator, image.image, image.vmaAlloc);
// }
