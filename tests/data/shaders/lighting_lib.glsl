
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
