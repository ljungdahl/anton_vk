#pragma once

#include <vulkan/vulkan.h>
#include "assert.h"

#define VK_CHECK(expr) { \
    ASSERT(expr == VK_SUCCESS); \
}
