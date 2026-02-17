#version 450

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec3 fragColor;
//layout(location = 1) in vec2 texCoord;

//layout(set = 0, binding = 0) uniform sampler _sampler;
//layout(set = 1, binding = 0) uniform texture2D _texture;

void main() {
    //outColor = texture(sampler2D(_texture, _sampler), texCoord);
    outColor = vec4(fragColor, 1.0);
}
