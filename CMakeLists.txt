cmake_minimum_required(VERSION 3.16)
project(anton_vk)

set(VULKAN_INCLUDE "C:\\VulkanSDK\\1.2.141.2\\Include")

set(GLFW_INCLUDE "C:\\lib\\glfw\\include")
set(GLM_INCLUDE "C:\\lib\\glm_include")

include_directories(${VULKAN_INCLUDE}
        ${GLFW_INCLUDE}
        ${GLM_INCLUDE}
        external src)

set(CMAKE_CXX_STANDARD 17)

add_executable(anton_vk
        src/logger.cpp
        src/vma.cpp
        src/main.cpp
        src/vk_base.cpp src/vk_base.h
        src/vk_device.cpp src/vk_device.h
        src/vk_common.h
        src/vk_surface.cpp src/vk_surface.h
        src/vk_swapchain.cpp src/vk_swapchain.h
        src/vk_resources.cpp src/vk_resources.h
        src/vk_render.cpp src/vk_render.h
        src/scene.cpp src/scene.h
        src/vk_renderprograms.cpp src/vk_renderprograms.h
        src/vertex_type.h)

find_package(Vulkan REQUIRED)

set(GLFW_LIBPATH "C:\\lib\\glfw\\lib-vc2019\\glfw3dll.lib")
target_link_libraries(anton_vk ${Vulkan_LIBRARY} ${GLFW_LIBPATH})