#version 450
#extension GL_ARB_separate_shader_objects : enable

#define NUM_LIGHTS 2

layout(location = 0) in vec3 Normal;
layout(location = 1) in vec3 wsVertex;
layout(location = 2) in vec3 light_dirs[NUM_LIGHTS];

layout(location = 0) out vec4 outColor;

//in gl_Position;

void main() {

    vec3 colorVec = vec3(1.0f, 0.0f, 1.0f);
    outColor = vec4(colorVec, 1.0f);
 
}	      			
