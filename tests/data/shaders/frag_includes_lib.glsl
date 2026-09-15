#ifndef  _FRAGMENT_INCLUDES
#define _FRAGMENT_INCLUDES

layout(location = 0) out vec4 outColor;

layout(location = 0) in VS_OUT
{
    vec3 fgNormal;
    vec2 fragTexCoord;
    vec3 fpos;
    mat3 fTBN;
    vec3 fTangent;
} fs_in;

#endif
