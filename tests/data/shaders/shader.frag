#version 460

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec3 fgNormal;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 fpos;

layout(set = 0, binding = 1) uniform sampler _sampler;
layout(set = 1, binding = 1) uniform texture2D _texture;

struct Light {
    vec3 color;
    vec3 direction;
    vec3 position;
};

layout(std140, set = 0, binding = 2) readonly buffer LightBlock {
    Light lights[];
} lightData;

float ambientI = 0.1;

void main() {
    vec3 L = normalize(fpos.xyz - lightData.lights[0].position);
    float diff = dot(L, normalize(fgNormal));
    diff = max(0., diff);
    vec3 diffColor = texture(sampler2D(_texture, _sampler), texCoord).rgb;
    outColor = vec4(diffColor * diff * lightData.lights[0].color + ambientI * diffColor, 1.);
}
