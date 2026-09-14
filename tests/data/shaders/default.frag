#version 460
#extension GL_GOOGLE_include_directive : enable

#include "fragment_includes.frag"
#include "light_functions.glsl"

// here you can add as many as you want
layout(std140, set = 1, binding = 0) uniform MatParms {
    vec3 color;
    float ambientI;
};

// here you declare the textures you need
//layout(set = 1, binding = 1) uniform texture2D _texture[2];

vec3 shadowSpotLight(int i) {
    // compute position from lights perspective
    vec4 cam_pos = lightData.lights[i].proj * vec4(fs_in.fpos, 1.0f);
    cam_pos /= cam_pos.w;
    vec2 coords = cam_pos.xy;
    vec3 L = lightData.lights[i].position - fs_in.fpos;
    float dist = length(L);
    L = normalize(L);
    float light = smoothstep(0, 0.1, 1 - length(coords)) * (1.0 / (dist * dist)) * lightData.lights[i].intensity * max(dot(L, lightData.lights[i].direction), 0);
    coords += 1.;
    coords /= 2.;
    coords.x /= 10; // alongated texture
    coords.x += lightData.lights[i].shadow_map * (1. / 10.); // move to the correct place
    if (lightData.lights[i].shadow_map >= 0 && light > 0.0f) {
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
        light *= shadow;
    }

    vec3 ilum = color * diffuse(L, normalize(fs_in.fgNormal)) * lightData.lights[i].color;
    return ilum * light;
}

void main() {
    vec3 lcolor = color * ambientI;

    vec3 V = normalize(ubo.obs - fs_in.fpos);

    for (int i = 0; i < ubo.nLights; i++) {
        lcolor += shadowSpotLight(i);
    }

    outColor = vec4(lcolor, 1.0f);
}
