#pragma once

#define WIN32_LEAN_AND_MEAN

#include "anton_asserts.h"
#include "logger.h"
#include "typedefs.h"

#define VK_USE_PLATFORM_WIN32_KHR

#include "vertex_type.h"

struct VPmatrices_t {
    glm::mat4 view;
    glm::mat4 proj;
};

struct Mesh_t {
    bool isStatic = false;
    std::vector<Vertex_t> vertices;
    std::vector<u32> indices;
    glm::mat4 modelMatrix;

    u32 vertexOffset = 0;
    u32 firstVertex = 0;

    u32 indexOffset = 0;
    u32 firstIndex = 0;

    u32 indexCount = 0;
};   

