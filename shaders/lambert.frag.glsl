#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 Normal;
layout(location = 1) in vec3 lightPos;
layout(location = 2) in vec3 wsVertex;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 n = normalize(Normal);
    vec3 light_dir = normalize(lightPos - wsVertex);
    float cosTheta = clamp( dot(n, light_dir), 0, 1);
    
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    float lightIntensity = .5f;
    vec3 light = lightIntensity * lightColor;

    vec3 baseDiffuse = {0.2, 0.2, 0.2};
    vec3 color = baseDiffuse +  light * cosTheta;
    
    outColor = vec4(color, 1.0);
}	      			


    // vec3 n = normalize(Normal);
    // vec3 view_dir = normalize(camPos - wsVertex);
    
    // float cosTheta = clamp( dot(n, view_dir), 0, 1);
    
    // vec3 lightColor = vec3(1.0, 1.0, 1.0);
    // float lightIntensity = .5f;
    // vec3 light = lightIntensity * lightColor;

    // vec3 baseDiffuse = {0.3, 0.3, 0.3};
    
    // vec3 color = baseDiffuse +  light * cosTheta;
    
    // outColor = vec4(color, 1.0);
