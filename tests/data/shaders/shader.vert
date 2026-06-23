#version 450

layout(push_constant) uniform pc {
    mat4 model;
};

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 obs;
    float time;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 0) out VS_OUT
{
    vec3 fgNormal;
    vec2 fragTexCoord;
    vec3 fpos;
    mat3 fTBN;
    vec3 fTangent;
} vs_out;

void main() {
    vec3 N = normalize(vec3(model * vec4(inNormal, 0.0f)));
    vec3 T = normalize(vec3(model * vec4(inTangent, 0.0f)));

    T = normalize(T - dot(T, N) * N);
    vec3 B = normalize(cross(N, T));

    vs_out.fTBN = mat3(T, B, N);

    vs_out.fpos = vec3(model * vec4(inPosition, 1.0f));
    vs_out.fragTexCoord = inTexCoord;
    vs_out.fTangent = T;
    vs_out.fgNormal = N;

    gl_Position = ubo.proj * ubo.view * model * vec4(inPosition, 1.0f);
}
