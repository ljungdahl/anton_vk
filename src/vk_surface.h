#pragma once

// fwd declare structs
struct VulkanContext_t;
struct GLFWwindow;

void createGLFWsurface(GLFWwindow* windowPtr, VulkanContext_t &context);