#version 450
#extension GL_ARB_separate_shader_objects : enable

#define NUM_LIGHTS 2

layout(location = 0) in vec3 Normal;
layout(location = 1) in vec3 wsVertex;
layout(location = 2) in vec3 light_dirs[NUM_LIGHTS];

layout(location = 0) out vec4 outColor;

void main() {
    vec3 n = normalize(Normal);
    float cosTheta = clamp( dot(n, light_dirs[0]), 0, 1);
    
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    float lightIntensity = .5f;
    vec3 light = lightIntensity * lightColor;

    vec3 baseDiffuse = {0.2, 0.2, 0.2};
    vec3 color = baseDiffuse +  light * cosTheta;
    
    outColor = vec4(color, 1.0);
}	      			
