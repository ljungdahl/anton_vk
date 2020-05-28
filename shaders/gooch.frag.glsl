#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {

    vec3 color = fragColor;
    outColor = vec4(color, 1.0);
}	      			
