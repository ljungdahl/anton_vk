#define GLFW_EXPOSE_NATIVE_WIN32

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "scene.h"
#include "vk_base.h"

std::vector<Mesh_t> meshes;
VPmatrices_t VPmatrices;

static bool uboBufferCreated = false;

void processKeyInput(GLFWwindow *windowPtr) {
    if (glfwGetKey(windowPtr, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(windowPtr, true);
    }
}

void sendStaticResources(std::vector<Mesh_t> &meshList) {
    for (auto mesh : meshList) {
        u32 vbSize = mesh.vertices.size() * sizeof(mesh.vertices[0]);
        u32 ibSize = mesh.indices.size() * sizeof(mesh.indices[0]);
        uploadVertices(vbSize, mesh.vertices.data());
        uploadIndices(ibSize, mesh.indices.data());
    }

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
    setupScene(meshes, VPmatrices, 1280, 720);
    Logger::Trace("meshes.modelMatrix[0], meshes.modelMatrix[1], meshes.modelMatrix[2]: %f %f %f",
            meshes[0].modelMatrix[0], meshes[0].modelMatrix[1], meshes[0].modelMatrix[2]);

    // Static resources
    sendStaticResources(meshes);

    // Setup first time renderprograms //TODO(anton): This is the bad way. Will redo completely.
    firstRenderPrograms();

    u32 imageIndex;
    u32 frameCounter = 0;
    f64 elapsedTime = 0.0f;
    f64 previousTime = 0.0f;
    glfwSetTime(elapsedTime);
    f64 deltaTime;

    Logger::Trace("meshes[0].indexCount: %i", meshes[0].indexCount);

    while (!glfwWindowShouldClose(windowPtr)) {
        glfwPollEvents();

        deltaTime = glfwGetTime() - previousTime;

        processKeyInput(windowPtr);

        // Begin render calls
        imageIndex = renderFrame(meshes[0].indexCount, elapsedTime);
        if(imageIndex == U32_MAX) continue; // swapchainStatus == Swapchain_NotReady

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