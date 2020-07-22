#include <vector>

#include "common.h"
#include "logger.h"
#include "vk_device.h"

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

    createInfo.enabledExtensionCount = ARRAYSIZE(extensions);
    createInfo.ppEnabledExtensionNames = extensions;

    const char* requiredValidationLayers[] = {"VK_LAYER_KHRONOS_validation"};

    // Now we fill in the createInfo with the required layer!
    createInfo.enabledLayerCount =  ARRAYSIZE(requiredValidationLayers);
    createInfo.ppEnabledLayerNames = requiredValidationLayers;

    // Finally we can create the instance
    VkInstance instance = 0;
    VK_CHECK( vkCreateInstance(&createInfo, nullptr, &instance) );

    return instance;
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
    gpu.gfxFamilyIndex = U32_MAX;
    gpu.presentFamilyIndex = U32_MAX;
    for (const auto& physicalDevice : physicalDevices)
    {
        VkPhysicalDeviceProperties deviceProps;
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProps);
        auto limits = deviceProps.limits;

        Logger::Trace("maxPushConstantSize %i", limits.maxPushConstantsSize);
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
            gpu.gfxFamilyIndex = graphicsIndex;
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
    queueInfo.queueFamilyIndex = gpu->gfxFamilyIndex;
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

    // We return VK_FALSE here since from Vulkan spec 1.2.133, ch. 39.1:
    // "The callback returns a VkBool32, which is interpreted in a layer-specified manner.
    // The application should always return VK_FALSE. The VK_TRUE value is reserved for use in layer development."
    return VK_FALSE;
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
