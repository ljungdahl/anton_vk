#pragma once

#define WIN32_LEAN_AND_MEAN

#include <vector>
#include <fstream>
#include <unordered_map>

#include "logger.h"
#include "typedefs.h"

#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan.h>
#include "anton_asserts.h"

#define VK_CHECK(expr) { \
    ASSERT(expr == VK_SUCCESS); \
}

#include "../external/vk_mem_alloc.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "../external/tiny_obj_loader.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
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


namespace std {
    // hash function for Vertex
    template<> struct hash<Vertex_t>
    {
        size_t operator()(Vertex_t const& vertex) const
        {
            return vertex.hash();
        }
    };
}


struct MVPmatrices_t {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct Mesh_t {
    std::vector<Vertex_t> vertices;
    std::vector<u32> indices;
    u32 indexCount = 0;
};   

struct VertexDescriptions_t {
    VkPipelineVertexInputStateCreateInfo inputState;
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
};
