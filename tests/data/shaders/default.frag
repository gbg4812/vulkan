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


void main() {
    vec3 lcolor = color * ambientI;

    vec3 V = normalize(ubo.obs - fs_in.fpos);

    for (int i = 0; i < ubo.nLights; i++) {
        lcolor += shadowSpotLight(i);
    }

    outColor = vec4(lcolor, 1.0f);
}
