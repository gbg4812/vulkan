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


vec3 spotLightTST(Light light, vec3 w_pos, vec3 w_n, float max_width) {
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

        float ocd_min = 10;
        float ocd_max = -10;
        for (int i = 0; i < search_pattern.length(); i++) {
            float closest = (texture(sampler2D(_shadow_map, _sampler), coords + search_pattern[i] * off * max_width)).r;
            ocd_min = min(ocd_min, closest - cam_pos.z);
            ocd_max = max(ocd_max, closest - cam_pos.z);
        }

        float shadow = 0.5;
        if ( ocd_max < 0.00075 && abs(ocd_min+ocd_max) < 0.0005) {
            shadow = 1;
        } else if(ocd_min + ocd_max < ocd_min * 1.7) {
            shadow = 0;
        } else {
            shadow = 0;
            float oc_f = max_width*2*abs(ocd_min)/cam_pos.z;
            int nsamples = sample_pattern.length();//int(max(sample_pattern.length()*oc_f, 1));
            for (int i = 0; i < nsamples; i++) {
                float closest = (texture(sampler2D(_shadow_map, _sampler), coords + sample_pattern[i] * off * max_width * oc_f)).r;
                if (closest >= cam_pos.z - (0.0005 + 0.00001*i)) {
                    shadow += 1. / nsamples;
                }
            }
        }
        

        bright *= shadow;
    }

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
