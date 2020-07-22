#version 450
#extension GL_ARB_separate_shader_objects : enable

#define NUM_LIGHTS 2

layout(binding = 0) uniform Uniforms_t {
   mat4 view;
   mat4 proj;
} ubo;

layout(push_constant) uniform PushConsts {
    mat4 model;
    vec4 lights[NUM_LIGHTS];
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec3 wsVertex;
layout(location = 2) out vec3 light_dirs[NUM_LIGHTS];


void main() {
    vec4 world_space_vertex = pc.model * vec4(inPosition, 1.0); //TODO(anton): Make sure ubo.model works!! !?!?!
    vec4 view_space_vertex = ubo.view * world_space_vertex;
    
    gl_Position = ubo.proj * view_space_vertex;
    //gl_Position = vec4(inPosition, 1.0);

    wsVertex = world_space_vertex.xyz;

    outNormal = mat3(transpose(inverse(pc.model))) * inNormal;
    //outNormal = inNormal;

    for (int i = 0; i < NUM_LIGHTS; ++i)
    {
        //light_dirs[i] = pc.matrices[1][i].xyz - world_space_vertex.xyz;
        light_dirs[i] = pc.lights[i].xyz - world_space_vertex.xyz;

    }
    //viewPos = pc.cameraPos.xyz;
    
}

