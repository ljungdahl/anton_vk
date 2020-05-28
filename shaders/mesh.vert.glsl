#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform MVPmatrices_t {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;

void main() {
    vec4 world_space_vertex = ubo.model * vec4(inPosition, 1.0);
    vec4 view_space_vertex = ubo.view * world_space_vertex;
    gl_Position = ubo.proj * view_space_vertex;

    fragColor = gl_Position.xyz;
}

