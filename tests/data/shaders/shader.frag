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
layout(set = 2, binding = 0) uniform texture2D _shadow_map;

struct Light {
    vec3 color;
    vec3 direction;
    vec3 position;
    mat4 proj;
    int shadow_map;
};

layout(std140, set = 0, binding = 2) readonly buffer LightBlock {
    Light lights[];
} lightData;

layout(std140, set = 1, binding = 0) uniform MatParms {
    vec3 color;
    bool normalMap;
    bool colorMap;
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
    vec2 tex_coords = fs_in.fragTexCoord;
    tex_coords.y = 1.0 - tex_coords.y;
    vec3 albedo = texture(sampler2D(_texture[0], _sampler), tex_coords).rgb * color;
    vec3 lcolor = vec3(0.0f);
    vec3 V = normalize(ubo.obs - fs_in.fpos);

    vec3 n = texture(sampler2D(_texture[1], _sampler), tex_coords).rgb;
    n = (n * 2.) - 1.;
    n.y *= -1;
    n = normalize(fs_in.fTBN * n);

    for (int i = 0; i < lightData.lights.length(); i++) {
        vec4 cam_pos = lightData.lights[i].proj * vec4(fs_in.fpos, 1.0f);
        cam_pos /= cam_pos.w;
        vec2 coords = cam_pos.xy;
        float shadow = smoothstep(0, 0.1, 1 - length(cam_pos.xy));
        coords += 1.;
        coords /= 2.;
        coords.x /= 10; // alongated texture
        coords.x += lightData.lights[i].shadow_map * (1. / 10.); // move to the correct place
        if (lightData.lights[i].shadow_map >= 0) {
            float d = (texture(sampler2D(_shadow_map, _sampler), coords)).r;
            if (d < cam_pos.z - 0.0001) {
                shadow = 0.0f;
            }
        }

        vec3 L = normalize(lightData.lights[i].position - fs_in.fpos);

        vec3 ilum = albedo * diffuse(L, n) * lightData.lights[i].color + (lightData.lights[i].color * spec(L, n, V, int(shaininess)));

        lcolor += ilum * shadow;
    }

    lcolor += ambientI * albedo;

    outColor = vec4(lcolor, 1.0f);
}
