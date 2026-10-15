#version 460
#extension GL_GOOGLE_include_directive : enable

#include "frag_includes_lib.glsl"
#include "global_includes_lib.glsl"
#include "lighting_lib.glsl"

// here you can add as many as you want
layout(std140, set = 1, binding = 0) uniform MatParms {
    vec3 color;
    float ambientI; // <0.01>
    float max_width; // <5.0>
};

// here you declare the textures you need
//layout(set = 1, binding = 1) uniform texture2D _texture[2];

const float trig_kernel[] = {
        0.1111111111111111,
        0.2222222222222222,
        0.3333333333333333,
        0.2222222222222222,
        0.1111111111111111
    };

float computeSoftShadow(Light light, vec3 w_pos, vec4 cam_pos, float max_width, vec3 w_n) {
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
        float closest = texture(sampler2D(_shadow_map, _sampler), coords + rotate2D(sample_pattern[i], rand_a) * off * max_width).r;
        if (closest < cam_pos.z - (0.0005 + 0.0001 * length(sample_pattern[i]))) {
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
        if (closest >= cam_pos.z - (0.0005 + 0.0001 * length(sample_pattern[i]))) {
            shadow += 1. / nsamples;
        }
    }

    return shadow;
}

float computeShadow(Light light, vec3 w_pos, vec4 cam_pos, float max_width, vec3 w_n) {
    vec3 L = normalize(light.position - w_pos);
    vec2 coords = cam_pos.xy;
    coords += 1.;
    coords /= 2.;
    coords.x /= 10; // alongated texture
    coords.x += light.shadow_map * (1. / 10.); // move to the correct place
    vec2 off = {
            (1. / (2048. * 10)),
            (1. / 2048.)
        };

    float shadow = 0;
    int kernel_width = 9;
    for (int i = 0; i < kernel_width; i++) {
        for (int j = 0; j < kernel_width; j++) {
            vec2 sampl = (vec2(i, j) - kernel_width / 2);
            float closest = texture(sampler2D(_shadow_map, _sampler), coords + sampl * off).r;
            if (closest >= cam_pos.z - (0.0005 + 0.001 * length(sampl))) {
                shadow += 1 / (pow(kernel_width, 2));
            }
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
    float z_p = ((-v_pos.z) - light.position.x) / (light.position.y - light.position.x);
    int l_idx = 0;
    if (z_p > 1.f / 8.f) {
        l_idx = 4 - int(ceil(log2(1.0f / z_p)));
    }
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

void main() {
    vec3 lcolor = color * ambientI;

    vec3 w_n = normalize(fs_in.fgNormal);

    for (int i = 0; i < ubo.nLights; i++) {
        Light light = lightData.lights[i];
        if (light.type == 0) {
            vec3 w_l = normalize(light.position - fs_in.fpos);
            lcolor += color * diffuse(w_l, w_n) * spotLight(light, fs_in.fpos, w_n, max_width);
        } else if (light.type == 1) {
            lcolor += color * diffuse(normalize(-light.direction), w_n) * dirLight(light, i, fs_in.fpos, w_n, max_width);
            i += 3;
        }
    }

    outColor = vec4(lcolor, 1.0f);
}
