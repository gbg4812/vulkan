#version 460

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec3 fragColor;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 obs;
    float time;
} ubo;



void main() {

    outColor = vec4(fragColor, 1.0f);
}
