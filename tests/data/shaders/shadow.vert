#version 450

layout(push_constant) uniform pc {
    mat4 model;
};

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 obs;
    float time;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;

layout(set = 1, binding = 0) uniform MatParms {
    int lightIndex;
};

struct Light {
    vec3 color;
    vec3 direction;
    vec3 position;
    mat4 proj;
};

layout(std140, set = 0, binding = 2) readonly buffer LightBlock {
    Light lights[];
} lightData;

void main() {
    gl_Position = lightData.lights[lightIndex].proj * model * vec4(inPosition, 1.0f);
}
