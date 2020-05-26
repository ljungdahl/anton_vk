#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 viewPos;
    vec3 lightPos;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 Normal;
layout(location = 2) out vec3 lightPos;
layout(location = 3) out vec3 wsVertex;
layout(location = 4) out vec3 viewPos;
void main() {
    vec4 world_space_vertex = ubo.model * vec4(inPosition, 1.0);
    vec4 view_space_vertex = ubo.view * world_space_vertex;
    gl_Position = ubo.proj * view_space_vertex;
    Normal = mat3(transpose(inverse(ubo.model))) * inNormal;
    
    lightPos = ubo.lightPos;
    
    wsVertex = world_space_vertex.xyz;
    
    viewPos = ubo.viewPos;
    
}

