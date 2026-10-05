#version 460
#extension GL_GOOGLE_include_directive : enable

#include "frag_includes_lib.glsl"
#include "global_includes_lib.glsl"
#include "lighting_lib.glsl"

// here you can add as many as you want
layout(std140, set = 1, binding = 0) uniform MatParms {
    vec3 color;
    float ambientI;
    float max_width;
};

// here you declare the textures you need
//layout(set = 1, binding = 1) uniform texture2D _texture[2];

float computeShadow(Light light, vec3 w_pos, vec4 cam_pos, float max_width) {
    vec3 L = normalize(light.position - w_pos);
    vec2 coords = cam_pos.xy;
    coords += 1.;
    coords /= 2.;
    coords.x /= 10; // alongated texture
    coords.x += light.shadow_map * (1. / 10.); // move to the correct place
    vec2 off = {
            (1. / (1080. * 10.)),
            (1. / 1080.)
        };

    float rand_a = sin(52253 * coords.x + 9709039 * coords.y);
    float oc_d = 0;
    int oc_n = 0;
    for (int i = 0; i < search_pattern.length(); i++) {
        float closest = (texture(sampler2D(_shadow_map, _sampler), coords + rotate2D(search_pattern[i], rand_a) * off * max_width)).r;
        if (closest < cam_pos.z - 0.0005) {
            oc_d += closest;
            oc_n += 1;
        }
    }

    float shadow = 0;
    oc_d = oc_d / oc_n;
    float oc_f = clamp(max_width * 2 * (cam_pos.z - oc_d) / cam_pos.z, 0.1, 1);
    int nsamples = int(sample_pattern.length() * oc_f);
    for (int i = 0; i < nsamples; i++) {
        float closest = (texture(sampler2D(_shadow_map, _sampler), coords + rotate2D(sample_pattern[i], rand_a) * off * max_width)).r;
        if (closest >= cam_pos.z - (0.0005 + 0.00001 * i)) {
            shadow += 1. / nsamples;
        }
    }

    return shadow;
}

vec3 spotLightTST(Light light, vec3 w_pos, vec3 w_n, float max_width) {
    // compute position from lights perspective
    vec3 L = light.position - w_pos;
    vec4 cam_pos = light.proj * vec4(w_pos, 1.0f);
    cam_pos /= cam_pos.w;
    float dist = length(L);
    // spot light mask and decay
    float bright = smoothstep(0, 0.1, 1 - length(cam_pos.xy)) * (1.0 / (dist * dist)) * light.intensity * max(dot(L, light.direction), 0);

    if(light.shadow_map > -1 && bright > 0)
        bright *= computeShadow(light, w_pos, cam_pos, max_width);

    return light.color * bright;
}

void main() {
    vec3 lcolor = color * ambientI;

    vec3 w_n = normalize(fs_in.fgNormal);

    for (int i = 0; i < ubo.nLights; i++) {
        Light light = lightData.lights[i];
        vec3 w_l = normalize(light.position - fs_in.fpos);
        lcolor += color * diffuse(w_l, w_n) * spotLightTST(light, fs_in.fpos, w_n, max_width);
    }

    outColor = vec4(lcolor, 1.0f);
}
