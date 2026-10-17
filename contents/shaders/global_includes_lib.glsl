#ifndef _GLOBAL_INCLUDES_LIB
#define _GLOBAL_INCLUDES_LIB

layout(std140, set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 obs;
    float time;
    int nLights;
} ubo;

layout(set = 0, binding = 1) uniform sampler _sampler;

#endif
