#version 450
#extension GL_ARB_separate_shader_objects : enable

#define NUM_LIGHTS 2

layout(binding = 0) uniform MVPmatrices_t {
   mat4 model;
   mat4 view;
   mat4 proj;
} ubo;

layout(push_constant) uniform PushConsts {
    vec4 lightPos[NUM_LIGHTS];
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec3 wsVertex;
layout(location = 2) out vec3 light_dirs[NUM_LIGHTS];

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    vec4 world_space_vertex = ubo.model * vec4(inPosition, 1.0);
    vec4 view_space_vertex = ubo.view * world_space_vertex;
    
    gl_Position = ubo.proj * view_space_vertex;
    wsVertex = world_space_vertex.xyz;

    outNormal = mat3(transpose(inverse(ubo.model))) * inNormal;

    for (int i = 0; i < NUM_LIGHTS; ++i)
    {
        light_dirs[i] = pc.lightPos[i].xyz - world_space_vertex.xyz;
    }
    
}

