#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "vk_common.h"
#include "vk_surface.h"

void createGLFWsurface(GLFWwindow* windowPtr, VulkanContext_t &context) {

    VK_CHECK( glfwCreateWindowSurface(context.instance, windowPtr, nullptr, &context.surface) );

}