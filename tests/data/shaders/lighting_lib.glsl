
#include "global_includes_lib.glsl"

#ifndef _LIGHTING_LIB
#define _LIGHTING_LIB

struct Light {
    vec3 color;
    vec3 direction;
    vec3 position;
    mat4 proj;
    float intensity;
    int shadow_map;
    int type;
};

layout(std430, set = 0, binding = 2) readonly buffer LightBlock {
    Light lights[];
} lightData;

layout(set = 2, binding = 0) uniform texture2D _shadow_map;

vec2 rotate2D(vec2 vec, float angle) {
    mat2 m = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));

    return vec * m;
}

const vec2 search_pattern[] = {
        {
            0.0,
            1.0
        },
        {
            0.7071067811865475,
            0.7071067811865476
        },
        {
            1.0,
            6.123233995736766e-17
        },
        {
            0.7071067811865476,
            -0.7071067811865475
        },
        {
            1.2246467991473532e-16,
            -1.0
        },
        {
            -0.7071067811865475,
            -0.7071067811865477
        },
        {
            -1.0,
            -1.8369701987210297e-16
        },
        {
            -0.7071067811865477,
            0.7071067811865474
        },
    };

const vec2 sample_pattern[] = {
        vec2(-0.94201624, -0.39906216),
        vec2(0.94558609, -0.76890725),
        vec2(-0.094184101, -0.92938870),
        vec2(0.34495938, 0.29387760),
        vec2(-0.91588581, 0.45771432),
        vec2(-0.81544232, -0.87912464),
        vec2(-0.38277543, 0.27676845),
        vec2(0.97484398, 0.75648379),
        vec2(0.44323325, -0.97511554),
        vec2(0.53742981, -0.47373420),
        vec2(-0.26496911, -0.41893023),
        vec2(0.79197514, 0.19090188),
        vec2(-0.24188840, 0.99706507),
        vec2(-0.81409955, 0.91437590),
        vec2(0.19984126, 0.78641367),
        vec2(0.14383161, -0.14100790)
    };

float computeShadow(Light light, vec3 w_pos, vec4 cam_pos, float max_width, vec3 w_n) {
    vec3 L = normalize(light.position - w_pos);
    vec2 coords = cam_pos.xy;
    coords += 1.;
    coords /= 2.;
    coords.x /= 10; // alongated texture
    coords.x += light.shadow_map * (1. / 10.); // move to the correct place
    vec2 off = {
            (1. / (2048. * 10.)),
            (1. / 2048.)
        };

    float rand_a = sin(52253 * coords.x + 9709039 * coords.y) * 6.283185;
    float oc_d = 0;
    int oc_n = 0;
    for (int i = 0; i < sample_pattern.length(); i++) {
        float closest = texture(sampler2D(_shadow_map, _sampler), coords + rotate2D(sample_pattern[i], rand_a) * off * max_width * max(0, dot(w_n, L))).r;
        if (closest < cam_pos.z - 0.0005) {
            oc_d += closest;
            oc_n += 1;
        }
    }

    float shadow = 0;
    oc_d = oc_d / oc_n;
    float oc_f = clamp(max_width * 2 * (cam_pos.z - oc_d) / cam_pos.z, 0.1, 1);
    int nsamples = sample_pattern.length();
    for (int i = 0; i < nsamples; i++) {
        float closest = texture(sampler2D(_shadow_map, _sampler), coords + rotate2D(sample_pattern[i], rand_a) * off * max_width * oc_f).r;
        if (closest >= cam_pos.z - (0.0005 + 0.0001 * i)) {
            shadow += 1. / nsamples;
        }
    }

    return shadow;
}

vec3 spotLight(Light light, vec3 w_pos, vec3 w_n, float max_width) {
    // compute position from lights perspective
    vec3 L = light.position - w_pos;
    vec4 cam_pos = light.proj * vec4(w_pos, 1.0f);
    cam_pos /= cam_pos.w;
    float dist = length(L);
    // spot light mask and decay
    float bright = smoothstep(0, 0.1, 1 - length(cam_pos.xy)) * (1.0 / (dist * dist)) * light.intensity * max(dot(L, -light.direction), 0);

    if (light.shadow_map > -1 && bright > 0)
        bright *= computeShadow(light, w_pos, cam_pos, max_width, w_n);

    return light.color * bright;
}

vec3 dirLight(Light light, int i, vec3 w_pos, vec3 w_n, float max_width) {
    // light.position in directional lights means (znear, zfar, _)
    vec4 v_pos = ubo.view * vec4(fs_in.fpos, 1.0f);
    int l_idx = int(sqrt(((-v_pos.z) - light.position.x) / (light.position.y - light.position.x)) * 4);
    Light slight = lightData.lights[i + l_idx];
    // compute position from lights perspective
    vec4 cam_pos = slight.proj * vec4(w_pos, 1.0f);
    cam_pos /= cam_pos.w;
    float bright = slight.intensity;

    if (slight.shadow_map > -1 && bright > 0)
        bright *= computeShadow(slight, w_pos, cam_pos, max_width, w_n);

    //return vec3(l_idx == 0, l_idx == 1 || l_idx == 3, l_idx == 2 || l_idx == 3);
    return slight.color * bright;
}

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

#endif
