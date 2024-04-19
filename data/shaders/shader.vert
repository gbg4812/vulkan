#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant, std430) uniform Model {
    mat4 model;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    // model matrix moves the object to the desired place in the world
    // view matrix orientates de scene to the view
    // proj matrix aplies perspective deformation
    gl_Position = ubo.proj * ubo.view * model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
