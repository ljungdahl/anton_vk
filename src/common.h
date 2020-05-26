#pragma once

#include <vector>
#include <algorithm>
#include <fstream>
#include <initializer_list>

#include "logger.h"
#include "typedefs.h"

#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan.h>
#include "assert.h"

#define VK_CHECK(expr) { \
    ASSERT(expr == VK_SUCCESS); \
}

// -----
// zeux array count helper
template <typename T, size_t Size>
char (*countof_helper(T (&_Array)[Size]))[Size];
#define COUNTOF(array) (sizeof(*countof_helper(array)) + 0)
// -----

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

template <class T>
void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct Vertex_t {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;

    bool operator==(const Vertex_t& other) const {
        return pos == other.pos;
    }

    size_t hash() const {
        size_t seed = 0;
        hash_combine(seed, pos);
        hash_combine(seed, normal);
        hash_combine(seed, texCoord);
        return seed;
    }
};

#include "../external/vk_mem_alloc.h"

#include <GLFW/glfw3.h>

