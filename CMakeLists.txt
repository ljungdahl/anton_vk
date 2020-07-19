cmake_minimum_required(VERSION 3.16)
project(anton_vk)

set(VULKAN_INCLUDE "C:\\VulkanSDK\\1.2.141.2\\Include")

set(GLFW_INCLUDE "C:\\lib\\glfw\\include")
set(GLM_INCLUDE "C:\\lib\\glm_include")

include_directories(${VULKAN_INCLUDE}
        ${GLFW_INCLUDE}
        ${GLM_INCLUDE}
        external src)

set(CMAKE_CXX_STANDARD 14)

add_executable(anton_vk
        src/anton_vk.cpp
        src/logger.cpp
        src/vma.cpp
        )

find_package(Vulkan REQUIRED)

set(GLFW_LIBPATH "C:\\lib\\glfw\\lib-vc2019\\glfw3dll.lib")
target_link_libraries(anton_vk ${Vulkan_LIBRARY} ${GLFW_LIBPATH})