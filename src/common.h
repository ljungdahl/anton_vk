#pragma once

#define WIN32_LEAN_AND_MEAN

#include <vector>

#include "logger.h"
#include "typedefs.h"

#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan.h>
#include "assert.h"

#define VK_CHECK(expr) { \
    ASSERT(expr == VK_SUCCESS); \
}

#include "../external/vk_mem_alloc.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
