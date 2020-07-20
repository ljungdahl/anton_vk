#pragma once

#include "vk_common.h"

VkInstance createInstance();
GPUInfo_t pickGPU(VkInstance instance, VkSurfaceKHR surface);
VkDevice createDevice(VkInstance instance, const GPUInfo_t* gpu);
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT              messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT                     messageTypes,
                                                    const VkDebugUtilsMessengerCallbackDataEXT*         pCallbackData,
                                                    void* pUserData);
void setupDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT* debugMessenger);