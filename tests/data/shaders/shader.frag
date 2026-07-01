#version 460

layout(location = 0) out vec4 outColor;

layout(location = 0) in VS_OUT
{
    vec3 fgNormal;
    vec2 fragTexCoord;
    vec3 fpos;
    mat3 fTBN;
    vec3 fTangent;
} fs_in;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 obs;
    float time;
} ubo;

layout(set = 0, binding = 1) uniform sampler _sampler;
layout(set = 1, binding = 1) uniform texture2D _texture[2];

struct Light {
    vec3 color;
    vec3 direction;
    vec3 position;
    mat4 proj;
};

layout(std140, set = 0, binding = 2) readonly buffer LightBlock {
    Light lights[];
} lightData;

layout(set = 1, binding = 0) uniform MatParms {
    vec3 color;
    float ambientI;
    float shaininess;
};

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
    vec3 albedo = texture(sampler2D(_texture[0], _sampler), fs_in.fragTexCoord).rgb * color;
    vec3 lcolor = ambientI * albedo;
    vec3 V = normalize(ubo.obs - fs_in.fpos);

    vec3 n = texture(sampler2D(_texture[1], _sampler), fs_in.fragTexCoord).rgb;
    n = (n * 2.) - 1.;
    n.y *= -1;
    n = normalize(fs_in.fTBN * n);

    for (int i = 0; i < lightData.lights.length(); i++) {
        vec3 L = normalize(lightData.lights[i].position - fs_in.fpos);

        lcolor += albedo * diffuse(L, n) * lightData.lights[i].color + (lightData.lights[i].color * spec(L, n, V, 127));
    }
    outColor = vec4(lcolor, 1.0f);
}
