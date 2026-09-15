#version 460
#extension GL_GOOGLE_include_directive : enable

#include "frag_includes_lib.glsl"
#include "global_includes_lib.glsl"
#include "lighting_lib.glsl"

// here you can add as many as you want
layout(std140, set = 1, binding = 0) uniform MatParms {
    vec3 color;
    float ambientI;
};

// here you declare the textures you need
//layout(set = 1, binding = 1) uniform texture2D _texture[2];


void main() {
    vec3 lcolor = color * ambientI;

    vec3 w_n = normalize(fs_in.fgNormal);

    for (int i = 0; i < ubo.nLights; i++) {
        Light light = lightData.lights[i];
        vec3 w_l = normalize(light.position - fs_in.fpos);
        lcolor += color * diffuse(w_l, w_n) * spotLight(light, fs_in.fpos, w_n);
    }

    outColor = vec4(lcolor, 1.0f);
}
