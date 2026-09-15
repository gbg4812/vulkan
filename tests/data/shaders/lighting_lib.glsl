
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
};

layout(std430, set = 0, binding = 2) readonly buffer LightBlock {
    Light lights[];
} lightData;

layout(set = 2, binding = 0) uniform texture2D _shadow_map;

vec3 spotLight(Light light, vec3 w_pos, vec3 w_n) {
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
        float shadow = 0;
        int fsize = 3;
        for (int s = 0; s < fsize * fsize; s++) {
            vec2 off = {
                    (s % fsize) * (1. / (1080. * 10.)),
                    (s / fsize) * (1. / 1080.)
                };
            float closest = (texture(sampler2D(_shadow_map, _sampler), coords + off)).r;
            if (closest >= cam_pos.z - 0.001) {
                shadow += 1. / (fsize * fsize);
            }
        }
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
