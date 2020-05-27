#pragma once

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

#include <GLFW/glfw3.h>

