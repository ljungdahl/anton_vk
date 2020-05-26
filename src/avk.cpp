
#define VSYNC 1

struct GPUInfo_t
{
    VkPhysicalDevice                    device;
    VkPhysicalDeviceFeatures            features;
    VkPhysicalDeviceProperties          props;
    VkPhysicalDeviceMemoryProperties    memProps;
    u32 graphicsFamilyIndex;
    u32 presentFamilyIndex;
};

struct Swapchain_t
{
    VkSwapchainKHR swapchain;
    std::vector<VkImage> images;
    u32 width, height;
    u32 imageCount;
};

enum SwapchainStatus
{
    Swapchain_Ready,
    Swapchain_Resized,
    Swapchain_NotReady,
};


#include "renderprog.cpp"
#include "resources.cpp"
/*
  =============
  debugCallback
  =============
*/
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT              messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT                     messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT*         pCallbackData,
    void* pUserData)
{

    switch (messageSeverity)
    {
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            Logger::Error(pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            Logger::Warn(pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            Logger::Log(pCallbackData->pMessage);
            break;
    }

    //std::cout << "VK_ValidationLayer: " << std::endl <<
    //    pCallbackData->pMessage << std::endl;
    
    // We return VK_FALSE here since from Vulkan spec 1.2.133, ch. 39.1:
    // "The callback returns a VkBool32, which is interpreted in a layer-specified manner.
    // The application should always return VK_FALSE. The VK_TRUE value is reserved for use in layer development."
    return VK_FALSE;
}

/*
  ===========================================================================

  vulkan initialisation

  ===========================================================================
*/
VkInstance createInstance()
{
    VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.pApplicationName = "anton_vk";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "TBA";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;
    
    VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    createInfo.pApplicationInfo = &appInfo;

    const char* extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WIN32_KHR        
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#ifdef _DEBUG
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };
    
    createInfo.enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]);
    createInfo.ppEnabledExtensionNames = extensions;
    
    const char* requiredValidationLayers[] = {"VK_LAYER_KHRONOS_validation"};
    
    // Now we fill in the createInfo with the required layer!    
    createInfo.enabledLayerCount =  sizeof(requiredValidationLayers) /
                                    sizeof(requiredValidationLayers[0]);
    createInfo.ppEnabledLayerNames = requiredValidationLayers;

    // Finally we can create the instance
    VkInstance instance = 0;
    VK_CHECK( vkCreateInstance(&createInfo, nullptr, &instance) );
      
    return instance;
}

void setupDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT* debugMessenger)
{
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
    vkCreateDebugUtilsMessengerEXT =
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>
        (vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    
    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI = {};
    debugUtilsMessengerCI.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugUtilsMessengerCI.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugUtilsMessengerCI.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugUtilsMessengerCI.pfnUserCallback = debugCallback;

    VK_CHECK( vkCreateDebugUtilsMessengerEXT(instance, &debugUtilsMessengerCI, nullptr, debugMessenger) );
    Logger::Trace("Created Vulkan debug layer.");
}

GPUInfo_t pickGPU(VkInstance instance, VkSurfaceKHR surface)
{
    // Get number of devices from first call to
    // vkEnumeratePhysicalDevices.
    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        
    if (deviceCount == 0)
    {
        Logger::Fatal("No physical devices found.");
    }

    // Get the actual available physical devices from the second call.
    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

    // Loop through the obtained devices and pick the first one that
    // satisfies our requirements.
    GPUInfo_t gpu;
    gpu.device = VK_NULL_HANDLE;
    gpu.graphicsFamilyIndex = U32_MAX;
    gpu.presentFamilyIndex = U32_MAX;
    for (const auto& physicalDevice : physicalDevices)
    {
        VkPhysicalDeviceProperties deviceProps;
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProps);
        Logger::Trace("Available device name: %s", deviceProps.deviceName);

        // See if we support graphics and present queue families
        u32 graphicsIndex = U32_MAX;
        u32 presentIndex = U32_MAX;
        u32 queueFamilyCount = 0;
        // First call gets a count
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
                                                 &queueFamilyCount,
                                                 nullptr);
        std::vector<VkQueueFamilyProperties> familyProperties(queueFamilyCount);
        // Second call gets the available queue families
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
                                                 &queueFamilyCount,
                                                 familyProperties.data());

        // For almost all cases graphics queue index and present queue
        // index coincide, ie the queues are the same. But we still
        // have to consider the case when this is not true. We also want to
        // present things to a surface and not only render to an offscreen buffer (I guess?).
        // So we have to check for presentation support explicitly regardless of whether the
        // present and graphics queues are of the same family.
        u32 i = 0;
        bool queueFamilyIndicesFound = false;
        for (const auto& property : familyProperties)
        {
            if (property.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                graphicsIndex = i;
            }

            VkBool32 presentSupport = false;
            // Note that we pass the current family index to see if
            // the family supports presentation.
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

            if (presentSupport)
            {
                presentIndex = i;
            }

            if (graphicsIndex != U32_MAX && presentIndex != U32_MAX)
            {
                queueFamilyIndicesFound = true;
                break;
            }

            i++;
        }

        // We get the available extensions and make sure they
        // match up to the required extensions of our choice.
        u32 extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

        std::vector<const char*> requiredExtensions;
        requiredExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        
        bool requiredExtensionsAreSupported = false;
        u32 extensionsMatch = 0;
        for (u32 i = 0; i < requiredExtensions.size(); i++)
        {
            for (u32 j = 0; j < extensionCount; j++)
            {
                if( strcmp(requiredExtensions[i], availableExtensions[j].extensionName) == 0)
                {
                    Logger::Trace("Matching extensions! required: %s, available: %s",
                                  requiredExtensions[i], availableExtensions[j].extensionName);
                    extensionsMatch += 1;
                }
            }
        }
    
        if(extensionsMatch == requiredExtensions.size())
        {
            requiredExtensionsAreSupported = true;
        }
    
        // We also want to query for the features, properties or memory properties we are
        // interested in. For now we only check for samplerAnisotropy (a feature).
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(physicalDevice, &features);

        bool physicalDeviceMeetsRequirements = (queueFamilyIndicesFound &&
                                                requiredExtensionsAreSupported &&
                                                features.samplerAnisotropy);

        if (physicalDeviceMeetsRequirements)
        {
            gpu.device = physicalDevice;
            Logger::Trace("Picked GPU: %s",
                          deviceProps.deviceName);
            // Get the props/memprops/features of the picked physical
            // device into the device member variable for future
            // reference.
            vkGetPhysicalDeviceFeatures(physicalDevice, &gpu.features);
            vkGetPhysicalDeviceProperties(physicalDevice, &gpu.props);
            vkGetPhysicalDeviceMemoryProperties(physicalDevice, &gpu.memProps);
            // Save the queue family indices for future reference
            gpu.graphicsFamilyIndex = graphicsIndex;
            gpu.presentFamilyIndex = presentIndex;
            break;
        }
    }
        
    if (gpu.device == VK_NULL_HANDLE)
    {
        Logger::Fatal("Failed to find suitable GPU.");
    }

    return gpu;
}

VkDevice createDevice(VkInstance instance, const GPUInfo_t* gpu)
{
    float queuePriorities[] = { 1.0f };

    VkDeviceQueueCreateInfo queueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    queueInfo.queueFamilyIndex = gpu->graphicsFamilyIndex;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = queuePriorities;

    std::vector<const char*> extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueInfo;

    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledExtensionCount = (u32)extensions.size();

    createInfo.pEnabledFeatures = &gpu->features;

    VkDevice device = 0;
    VK_CHECK( vkCreateDevice(gpu->device, &createInfo, nullptr, &device) );

    return device;
}

VkFormat getSwapchainFormat(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    // Here we init formats, image usage flags.
    ASSERT( surface != VK_NULL_HANDLE );
    u32 formats_count;
    VK_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR(device,
                                                   surface, &formats_count, nullptr) );
    ASSERT( formats_count > 0 );
    std::vector<VkSurfaceFormatKHR> surface_formats(formats_count);
    VK_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR(device,
                                                   surface, &formats_count,
                                                   surface_formats.data()) );

    
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

VkSemaphore createSemaphore(VkDevice device)
{
    VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    VkSemaphore semaphore = 0;
    VK_CHECK(vkCreateSemaphore(device, &createInfo, 0, &semaphore));

    return semaphore;
}

VkRenderPass createRenderPass(VkDevice device, VkFormat colorFormat, VkFormat depthFormat) {
    VkAttachmentDescription attachments[2] = {};

    attachments[0].format = colorFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachments[1].format = depthFormat;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef= { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkAttachmentReference depthAttachmentRef = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorAttachmentRef;
    subpassDescription.pDepthStencilAttachment = &depthAttachmentRef;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;
    subpassDescription.pResolveAttachments = nullptr;

    // Subpass dependencies for layout transitions
    VkSubpassDependency dependencies[2];

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    //dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    //dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDescription;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies;

    VkRenderPass renderPass = 0;
    VK_CHECK( vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) );
    
    return renderPass;
} 

VkCommandPool createCommandPool(VkDevice device, u32 familyIndex)
{
    VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    createInfo.queueFamilyIndex = familyIndex;

    VkCommandPool commandPool = 0;
    VK_CHECK(vkCreateCommandPool(device, &createInfo, 0, &commandPool));

    return commandPool;
}

void destroySwapchain(VkDevice device, const Swapchain_t& swapchain)
{
    vkDestroySwapchainKHR(device, swapchain.swapchain, 0);
}



static
VkSwapchainKHR createSwapchain(VkDevice device, VkSurfaceKHR surface,
                                      VkSurfaceCapabilitiesKHR surfaceCaps, u32 familyIndex,
                                      VkFormat format, u32 width, u32 height,
                                      VkSwapchainKHR oldSwapchain)
{

    u32 minImageCount = surfaceCaps.minImageCount + 1;
    if(surfaceCaps.maxImageCount > 0 && minImageCount > surfaceCaps.maxImageCount) {
        minImageCount = surfaceCaps.maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    createInfo.surface = surface;
    createInfo.minImageCount = minImageCount;
    createInfo.imageFormat = format;
    createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    createInfo.imageExtent.width = width;
    createInfo.imageExtent.height = height;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = &familyIndex;
    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = VSYNC ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
    createInfo.oldSwapchain = oldSwapchain;

    VkSwapchainKHR swapchain = 0;
    VK_CHECK( vkCreateSwapchainKHR(device, &createInfo, 0, &swapchain) );

    return swapchain;
}

void createSwapchain(Swapchain_t& result, VkPhysicalDevice physicalDevice,
                     VkDevice device, VkSurfaceKHR surface, u32 familyIndex,
                     VkFormat format, VkSwapchainKHR oldSwapchain)
{
    VkSurfaceCapabilitiesKHR surfaceCaps;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps));

    u32 width = surfaceCaps.currentExtent.width;
    u32 height = surfaceCaps.currentExtent.height;

    VkSwapchainKHR swapchain = createSwapchain(device, surface, surfaceCaps,
                                               familyIndex, format, width, height,
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

SwapchainStatus updateSwapchain(Swapchain_t& result, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, uint32_t familyIndex, VkFormat format)
{
    VkSurfaceCapabilitiesKHR surfaceCaps;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps));

    u32 newWidth = surfaceCaps.currentExtent.width;
    u32 newHeight = surfaceCaps.currentExtent.height;

    if (newWidth == 0 || newHeight == 0)
        return Swapchain_NotReady;

    if (result.width == newWidth && result.height == newHeight)
        return Swapchain_Ready;

    Swapchain_t old = result;

    createSwapchain(result, physicalDevice, device, surface, familyIndex, format, old.swapchain);

    VK_CHECK(vkDeviceWaitIdle(device));

    destroySwapchain(device, old);

    return Swapchain_Resized;
}
