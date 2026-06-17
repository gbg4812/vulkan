#version 460

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec3 fgNormal;
layout(location = 1) in vec2 texCoord;

layout(set = 0, binding = 1) uniform sampler _sampler;
layout(set = 1, binding = 1) uniform texture2D _texture;

struct Light {
    vec3 color;
    vec3 direction;
    vec3 position;
};

layout(std430, set = 0, binding = 2) readonly buffer LightBlock {
    Light lights[];
} lightData;

float ambientI = 0.1;

void main() {
    outColor = vec4(texture(sampler2D(_texture, _sampler), texCoord).rgb * (max(0, dot(normalize(gl_FragCoord.xyz - lightData.lights[0].position), normalize(fgNormal))) + ambientI), 1.0f);
}
