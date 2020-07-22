#define GLFW_EXPOSE_NATIVE_WIN32

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "scene.h"
#include "vk_base.h"

std::vector<Mesh_t> g_meshes;
VPmatrices_t g_VPmatrices;

static bool uboBufferCreated = false;

void processKeyInput(GLFWwindow *windowPtr) {
    if (glfwGetKey(windowPtr, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(windowPtr, true);
    }
}

void sendStaticResources(std::vector<Mesh_t> &meshList) {
    u32 totalVertexSize = 0;
    u32 totalIndexSize = 0;
    u32 vbOffset = 0;
    u32 vbSize = 0;
    u32 ibOffset = 0;
    u32 ibSize = 0;
    u32 meshCount = 0;
    for (auto &mesh : meshList) {
        vbSize = mesh.vertices.size() * sizeof(mesh.vertices[0]);
        totalVertexSize += vbSize;
        mesh.vertexOffset = vbOffset;
        mesh.firstVertex = mesh.vertexOffset / sizeof(mesh.vertices[0]);

        Logger::Trace("vbOffset = %i for mesh %i", mesh.vertexOffset, meshCount);
        Logger::Trace("firstVertex = %i for mesh %i", mesh.firstVertex, meshCount);

        vbOffset += vbSize; // The offset of the next mesh will be the size of the current one

        ibSize = mesh.indices.size() * sizeof(mesh.indices[0]);
        totalIndexSize += ibSize;
        mesh.indexOffset = ibOffset;
        mesh.firstIndex = mesh.indexOffset / sizeof(mesh.indices[0]);
        Logger::Trace("ibOffset = %i for mesh %i", mesh.indexOffset, meshCount);
        Logger::Trace("firstIndex = %i for mesh %i", mesh.firstIndex, meshCount);
        ibOffset += ibSize;

        meshCount += 1;

    }

    Logger::Trace("total vertex size: %i", totalVertexSize);
    Logger::Trace("total index size: %i", totalIndexSize);

    createStaticBuffers(totalVertexSize, totalIndexSize);

    meshCount = 0;
    for (auto &mesh : meshList) {
        u32 vertexSize = mesh.vertices.size() * sizeof(mesh.vertices[0]);
        u32 indexSize = mesh.indices.size() * sizeof(mesh.indices[0]);
        uploadVertices(vertexSize, mesh.vertexOffset, mesh.vertices.data());
        uploadIndices(indexSize, mesh.indexOffset, mesh.indices.data());
        meshCount += 1;
    }
}

u32 render(f64 time, std::vector<Mesh_t> &meshList) {

    u32 imageIndex = avk_prepareFrame(time);
    if (imageIndex == U32_MAX) return imageIndex;
    u32 meshIndex = 0;
    uploadUniformData(g_VPmatrices.view, g_VPmatrices.proj);
    for (Mesh_t &mesh : meshList) {
        uploadModelMatrix(mesh.modelMatrix);
        avk_drawMesh(mesh.firstVertex, mesh.firstIndex, mesh.indexCount, time); //TODO(anton): Material handle?
        meshIndex += 1;
    }

    avk_endFrame();

    return imageIndex;
}

i32 main(i32 argc, const char **argv) {
#ifdef _DEBUG
    Logger::Trace("_DEBUG defined.");
#endif
#ifdef _WIN32
    Logger::Trace("_WIN32 defined.");
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
    Logger::Trace("VK_USE_PLATFORM_WIN32_KHR defined.");
#endif

    // Init GLFW
    i32 rc = glfwInit();
    ASSERT(rc == GLFW_TRUE);
    ASSERT(glfwVulkanSupported() == GLFW_TRUE);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow *windowPtr;
    windowPtr = glfwCreateWindow(1280, 720, "anton_vk", nullptr, nullptr);

    // Init Vulkan
    initialiseVulkan(windowPtr);

    // Init scene
    setupScene(g_meshes, g_VPmatrices, 1280, 720);
    sendStaticResources(g_meshes);

    u32 imageIndex = 0;
    u32 frameCounter = 0;
    f64 elapsedTime = 0.0f;
    f64 previousTime = 0.0f;
    glfwSetTime(elapsedTime);
    f64 deltaTime = 0;

    f32 degs = 0.0f, degs2 = 0.0f;
    while (!glfwWindowShouldClose(windowPtr)) {
        glfwPollEvents();

        deltaTime = glfwGetTime() - previousTime;

        // rotate mesh 1

        if(g_meshes.size() > 1) {
            f32 step = 1.0f;
            degs = deltaTime * step;
            degs2 = deltaTime * 0.1f*step;
            glm::vec3 rotDir = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 rotDir2 = glm::vec3(1.0f, 0.0f, 1.0f);
            glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), degs, rotDir);
            glm::mat4 rotMat2 = glm::rotate(glm::mat4(1.0f), degs2, rotDir2);

            g_meshes[1].modelMatrix = rotMat2 * rotMat * g_meshes[1].modelMatrix;
        }

        processKeyInput(windowPtr);

        // Begin render calls
        imageIndex = render(elapsedTime, g_meshes);
        if (imageIndex == U32_MAX) continue;

        // End render calls
        previousTime = elapsedTime;
        elapsedTime = glfwGetTime();
        frameCounter++;
        char title[256];
        sprintf(title, "frame: %i - imageIndex: %i - delta time: %f - elapsed time: %f",
                frameCounter, imageIndex, deltaTime, elapsedTime);
        glfwSetWindowTitle(windowPtr, title);
    }

    if (windowPtr) {
        glfwDestroyWindow(windowPtr);
    }
    glfwTerminate();

    return 0;
}