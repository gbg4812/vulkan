#version 460

#extension GL_GOOGLE_include_directive : enable

#include "global_includes_lib.glsl" 
#include "vert_includes_lib.glsl"

void main() {
    vec3 N = normalize(vec3(model * vec4(inNormal, 0.0f)));
    vec3 T = normalize(vec3(model * vec4(inTangent, 0.0f)));

    T = normalize(T - dot(T, N) * N);
    vec3 B = normalize(cross(N, T));

    vs_out.fTBN = mat3(T, B, N);

    vs_out.fpos = vec3(model * vec4(inPosition, 1.0f));
    vs_out.fragTexCoord = inTexCoord;
    vs_out.fragTexCoord.y = 1.0 - vs_out.fragTexCoord.y;
    vs_out.fTangent = T;
    vs_out.fgNormal = N;

    gl_Position = ubo.proj * ubo.view * model * vec4(inPosition, 1.0f);
}
