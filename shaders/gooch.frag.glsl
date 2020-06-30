#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 Normal;
layout(location = 1) in vec3 camPos;
layout(location = 2) in vec3 lightPos;
layout(location = 3) in vec3 wsVertex;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 n = normalize(Normal);
    n = -n;
    vec3 view_dir = normalize(camPos - wsVertex);
    vec3 light_dir = normalize(lightPos - wsVertex);
    float cosTheta = clamp( dot(n, light_dir), 0, 1);

    vec3 c_surface = vec3(0.4, 0.4, 0.4);
    
    vec3 c_cool = vec3(0.0f, 0.0f, 0.55f) + 0.25f * c_surface;
    vec3 c_warm = vec3(0.5f, 0.2f, 0.0f) + 0.25f * c_surface;
    vec3 c_highlight = vec3(0.97f, 0.97f, 0.97f);
    float t = (cosTheta + 1.0f)/2.0f;
    vec3 rr = reflect(light_dir, n); // Calcs reflection on other side of normal based on incidence
    float s = clamp( (100.0f*dot(rr,view_dir)-97.0f), 0, 1);

    vec3 c_diffuse = mix(c_cool, c_warm, t); // mix(x,y,t) is just lerp: (1-t)*x+y*t
    vec3 color = mix(c_diffuse, c_highlight, s);
    
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
