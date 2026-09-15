#version 460

#extension GL_GOOGLE_include_directive : enable

#include "global_includes_lib.glsl" 
#include "lighting_lib.glsl"
#include "frag_includes_lib.glsl"


layout(set = 1, binding = 1) uniform texture2D _texture[2];

layout(std140, set = 1, binding = 0) uniform MatParms {
    vec3 color;
    float ambientI;
    float shaininess;
};


void main() {
    vec2 tex_coords = fs_in.fragTexCoord;
    vec3 albedo = texture(sampler2D(_texture[0], _sampler), tex_coords).rgb * color;
    vec3 lcolor = ambientI * albedo;
    
    vec3 w_V = normalize(ubo.obs - fs_in.fpos);

    vec3 n = texture(sampler2D(_texture[1], _sampler), tex_coords).rgb;
    n = (n * 2.) - 1.;
    n.y *= -1;
    n = normalize(fs_in.fTBN * n);

    for (int i = 0; i < ubo.nLights; i++) {
        Light light = lightData.lights[i];
        vec3 w_L = normalize(light.position - fs_in.fpos);
        vec3 ilum = albedo * diffuse(w_L, n)  +  spec(w_L, n, w_V, int(shaininess));
        
        lcolor += ilum * spotLight(light, fs_in.fpos, n);
    }


    outColor = vec4(lcolor, 1.0f);
}
