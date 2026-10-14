
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

vec3 spotLight(Light light, vec3 w_pos, vec3 w_n, float max_width) {
    const float PI = 3.141592653589;
    // compute position from lights perspective
    vec4 cam_pos = light.proj * vec4(w_pos, 1.0f);
    cam_pos /= cam_pos.w;
    vec2 coords = cam_pos.xy;
    vec3 L = light.position - w_pos;
    float dist = length(L);
    L = normalize(L);
    float bright = smoothstep(0, 0.1, 1 - length(coords)) * (1.0 / (dist * dist)) * light.intensity * max(dot(L, light.direction), 0);
    coords += 1.;
    coords /= 2.;
    coords.x /= 10; // alongated texture
    coords.x += light.shadow_map * (1. / 10.); // move to the correct place
    if (light.shadow_map >= 0 && bright > 0.0f) {
        vec2 off = {
                (1. / (1080. * 10.)),
                (1. / 1080.)
            };

        float ocd_min = 0;
        float ocd_max = 0;
        for (int i = 0; i < search_pattern.length(); i++) {
            float closest = (texture(sampler2D(_shadow_map, _sampler), coords + search_pattern[i] * off * max_width)).r;
            ocd_min = min(ocd_min, closest - cam_pos.z);
            ocd_max = max(ocd_max, closest - cam_pos.z);
        }

        float shadow = 0.5;
        if (abs(ocd_min + ocd_max) < 0.0005) {
            shadow = 1;
        } else if (abs(ocd_min + ocd_max) > ocd_max * 1.5) {
            shadow = 0;
        } /*else {
                    shadow = 0;
                    for (int i = 0; i < sample_pattern.length(); i++) {
                        float closest = (texture(sampler2D(_shadow_map, _sampler), coords + sample_pattern[i] * off * max_width)).r;
                        if (closest >= cam_pos.z - 0.0005) {
                            shadow += 1. / (sample_pattern.length());
                        }
                    }
                    }*/

        bright *= shadow;
    }

    return light.color * bright;
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
