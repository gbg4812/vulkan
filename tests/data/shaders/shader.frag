#version 460

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec3 fgNormal;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 fpos;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 obs;
    float time;
} ubo;

layout(set = 0, binding = 1) uniform sampler _sampler;
layout(set = 1, binding = 1) uniform texture2D _texture;

struct Light {
    vec3 color;
    vec3 direction;
    vec3 position;
};

layout(std140, set = 0, binding = 2) readonly buffer LightBlock {
    Light lights[];
} lightData;

layout(set = 1, binding = 0) uniform MatParms {
    vec3 color;
};

float ambientI = 0.1;

float diffuse(vec3 L, vec3 N) {
    float diff = dot(L, normalize(N));
    diff = max(0., diff);
    return diff;
}

float spec(vec3 L, vec3 N, vec3 V, int exp) {
    vec3 R = normalize(reflect(-L, N));
    float VdotR = max(0, dot(V, R));
    return pow(VdotR, exp);
}

void main() {
    vec3 L = normalize(lightData.lights[0].position - fpos);
    vec3 V = normalize(ubo.obs - fpos);

    vec3 albedo = texture(sampler2D(_texture, _sampler), texCoord).rgb * color;

    outColor = vec4(albedo * diffuse(L, fgNormal) * lightData.lights[0].color + ambientI * albedo + vec3(1.0f) * spec(L, fgNormal, V, 64), 1.);
}
