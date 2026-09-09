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

layout(std140, set = 1, binding = 0) uniform MatParms {
    vec3 color;
    float ambientI;
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
    vec3 albedo = color;
    vec3 lcolor = vec3(0.0f);
    vec3 V = normalize(ubo.obs - fs_in.fpos);

    for (int i = 0; i < ubo.nLights; i++) {
        vec4 cam_pos = lightData.lights[i].proj * vec4(fs_in.fpos, 1.0f);
        cam_pos /= cam_pos.w;
        vec2 coords = cam_pos.xy;
        vec3 L = lightData.lights[i].position - fs_in.fpos;
        float dist = length(L);
        L = normalize(L);
        float light = smoothstep(0, 0.1, 1 - length(coords)) * (1.0/(dist*dist)) * lightData.lights[i].intensity * max(dot(L, lightData.lights[i].direction), 0);
        coords += 1.;
        coords /= 2.;
        coords.x /= 10; // alongated texture
        coords.x += lightData.lights[i].shadow_map * (1. / 10.); // move to the correct place
        if (lightData.lights[i].shadow_map >= 0 && light > 0.0f) {
            float shadow = 0;
            int fsize = 3;
            for(int s = 0; s < fsize*fsize; s++) {
                vec2 off = {(s%fsize)*(1./(1080.*10.)), (s/fsize)* (1./1080.)};
                float closest = (texture(sampler2D(_shadow_map, _sampler), coords + off )).r;
                if (closest  >= cam_pos.z - 0.001) {
                    shadow += 1./(fsize*fsize);
                }
            }
            light *= shadow;
        }

        vec3 ilum = albedo * diffuse(L, normalize(fs_in.fgNormal)) * lightData.lights[i].color;

        lcolor += ilum * light;
    }

    lcolor += ambientI * albedo;

    outColor = vec4(lcolor, 1.0f);
}
