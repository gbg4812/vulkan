
#version 460

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform MatParms {
    vec3 color;
};

void main() {
    outColor = vec4(color, 1.0f);
}
