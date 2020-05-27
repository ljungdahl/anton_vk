#define VSYNC 1

struct Swapchain_t
{
    VkSwapchainKHR swapchain;
    std::vector<VkImage> images;
    u32 width, height;
    u32 imageCount;
};

enum SwapchainStatus_t
{
    Swapchain_Ready,
    Swapchain_Resized,
    Swapchain_NotReady,
};

VkFormat getSwapchainFormat(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    ASSERT( surface != VK_NULL_HANDLE );
    u32 formats_count;
    VK_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formats_count, nullptr) );
    ASSERT( formats_count > 0 );
    std::vector<VkSurfaceFormatKHR> surface_formats(formats_count);
    VK_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formats_count, surface_formats.data()) );
    
    if(surface_formats.size() == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED)
    {
        return VK_FORMAT_R8G8B8A8_UNORM;
    }
    
    for( VkSurfaceFormatKHR &surface_format : surface_formats )
    {
        if( surface_format.format == VK_FORMAT_R8G8B8A8_UNORM )
        {
            return surface_format.format;
        }
    }

    return surface_formats[0].format;
}


void destroySwapchain(VkDevice device, const Swapchain_t& swapchain)
{
    vkDestroySwapchainKHR(device, swapchain.swapchain, 0);
}


static
VkSwapchainKHR createSwapchainKHR(VkDevice device, VkSurfaceKHR surface,
                                      VkSurfaceCapabilitiesKHR surfaceCaps,
                                      VkFormat format, u32 width, u32 height,
                                      VkSwapchainKHR oldSwapchain)
{
    VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    createInfo.surface = surface;
    createInfo.minImageCount = 2;
    createInfo.imageFormat = format;
    createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    createInfo.imageExtent = {width, height};
    createInfo.imageArrayLayers = 1; // REQUIRED
    createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    //TODO(anton): Mailbox instead of FIFO? FIFO is more widely supported.
    createInfo.presentMode = VSYNC ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; // REQUIRED
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // REQUIRED

    VkSwapchainKHR swapchain = 0;
    VK_CHECK( vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain) );
    return swapchain;
}

void createSwapchain(Swapchain_t& result, VkPhysicalDevice physicalDevice,
                     VkDevice device, VkSurfaceKHR surface,
                     VkFormat format, VkSwapchainKHR oldSwapchain)
{
    VkSurfaceCapabilitiesKHR surfaceCaps;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps));

    u32 width = surfaceCaps.currentExtent.width;
    u32 height = surfaceCaps.currentExtent.height;

    VkSwapchainKHR swapchain = createSwapchainKHR(device, surface, surfaceCaps,
                                                  format, width, height,
                                                  oldSwapchain);
    ASSERT(swapchain != VK_NULL_HANDLE);

    u32 imageCount = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, 0));

    std::vector<VkImage> images(imageCount);
    VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data()));

    result.swapchain = swapchain;
    result.images = images;
    result.width = width;
    result.height = height;
    result.imageCount = imageCount;
}

SwapchainStatus_t updateSwapchain(Swapchain_t& swapchain, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, VkFormat format)
{
    
    VkSurfaceCapabilitiesKHR surfaceCaps;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps));

    u32 newWidth = surfaceCaps.currentExtent.width;
    u32 newHeight = surfaceCaps.currentExtent.height;

    if(newWidth == 0 || newHeight == 0)
    {
        return Swapchain_NotReady;
    }

    if(newWidth == swapchain.width && newHeight == swapchain.height)
    {
        return Swapchain_Ready; // Nothing changed.
    }
    
    // ELSE: Swapchain resized! We have to recreate.
    Swapchain_t oldSwapchain = swapchain;    
    createSwapchain(swapchain, physicalDevice, device, surface, format, oldSwapchain.swapchain);
    
    VK_CHECK( vkDeviceWaitIdle(device) ); //Make sure we aren't using old swapchain before destroying 

    destroySwapchain(device, oldSwapchain);
    
    return Swapchain_Resized;
}
