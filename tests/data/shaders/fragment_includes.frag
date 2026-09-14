layout(location = 0) out vec4 outColor;

layout(location = 0) in VS_OUT
{
    vec3 fgNormal;
    vec2 fragTexCoord;
    vec3 fpos;
    mat3 fTBN;
    vec3 fTangent;
} fs_in;

layout(std140, set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 obs;
    float time;
    int nLights;
} ubo;

layout(set = 0, binding = 1) uniform sampler _sampler;
layout(set = 2, binding = 0) uniform texture2D _shadow_map;

struct Light {
    vec3 color;
    vec3 direction;
    vec3 position;
    mat4 proj;
    float intensity;
    int shadow_map;
};

layout(std430, set = 0, binding = 2) readonly buffer LightBlock {
    Light lights[];
} lightData;
