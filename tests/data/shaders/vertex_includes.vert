layout(push_constant, std430) uniform pc {
    mat4 model;
};

layout(std140, set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 obs;
    float time;
    int nLights;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 0) out VS_OUT
{
    vec3 fgNormal;
    vec2 fragTexCoord;
    vec3 fpos;
    mat3 fTBN;
    vec3 fTangent;
} vs_out;
