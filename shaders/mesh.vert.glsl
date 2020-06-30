#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform MVPmatrices_t {
   mat4 model;
   mat4 view;
   mat4 proj;
} ubo;

layout(push_constant) uniform PushConsts {
    vec3 lightPos;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec3 lightPos;
layout(location = 2) out vec3 wsVertex;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    vec4 world_space_vertex = ubo.model * vec4(inPosition, 1.0);
    vec4 view_space_vertex = ubo.view * world_space_vertex;
    
    gl_Position = ubo.proj * view_space_vertex;

    outNormal = mat3(transpose(inverse(ubo.model))) * inNormal;
    //outNormal = mat3(ubo.model) * inNormal;
    //outNormal = inNormal;
    lightPos = pc.lightPos;

    wsVertex = world_space_vertex.xyz;

}

