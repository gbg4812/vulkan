#version 450

layout(push_constant) uniform pc {
    mat4 model;
};

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 objs;
    float time;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 0) out vec3 fgNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fpos;

void main() {
    fpos = vec3(model * vec4(inPosition, 1.0f));
    gl_Position = ubo.proj * ubo.view * model * vec4(inPosition, 1.0f);
    fgNormal = inNormal;
    fragTexCoord = inTexCoord;
}
