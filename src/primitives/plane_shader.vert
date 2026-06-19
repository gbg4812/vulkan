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

layout(location = 0) out vec3 fragColor;

layout(set = 1, binding = 0) uniform MatParms {
    vec3 color;
};

void main() {
    gl_Position = ubo.proj * ubo.view * model * vec4(inPosition, 1.0f);
    fragColor = color;
}
