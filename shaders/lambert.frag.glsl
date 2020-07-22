#version 450
#extension GL_ARB_separate_shader_objects : enable

#define NUM_LIGHTS 2

layout(location = 0) in vec3 Normal;
layout(location = 1) in vec3 wsVertex;
layout(location = 2) in vec3 light_dirs[NUM_LIGHTS];

layout(location = 0) out vec4 outColor;

void main() {
    vec3 n = normalize(Normal);
    //vec3 light_dirs[NUM_LIGHTS];
    //light_dirs[0] = vec3(-1.0f, 1.0f, 8.0f);
    //light_dirs[1] = vec3(0.0f, 1.0f, -3.0f);

    float cosTheta0 = clamp( dot(n, normalize(light_dirs[0])), 0, 1);
    float cosTheta1 = clamp( dot(n, normalize(light_dirs[1])), 0, 1);

    vec3 light0Color = vec3(1.0, 1.0, 1.0);
    vec3 light1Color = vec3(0.5, 0.0, 1.0);

    float lightIntensity0 = 0.5f;
    float lightIntensity1 = 1.0f;
    vec3 light0 = lightIntensity0 * light0Color;
    vec3 light1 = lightIntensity1 * light1Color;
    
    vec3 baseDiffuse = {0.2, 0.2, 0.2};
    vec3 color = baseDiffuse +  (light0 * cosTheta0) + (light1 * cosTheta1) ;
    
    outColor = vec4(color, 1.0);
}	      			
