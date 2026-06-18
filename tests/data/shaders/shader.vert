#version 450

layout(push_constant) uniform pc {
    mat4 model;
};

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(set = 1, binding = 0) uniform MatParms {
    vec3 color;
    float intensity;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fpos;

void main() {
    // model matrix moves the object to the desired place in the world
    // view matrix orientates de scene to the view
    // proj matrix aplies perspective deformation
    fpos = inPosition;
    gl_Position = ubo.proj * ubo.view * vec4(inPosition, 1.0f);
    //gl_Position = vec4(inPosition, 1.0);
    fragColor = inNormal;
    fragTexCoord = inTexCoord;
}
