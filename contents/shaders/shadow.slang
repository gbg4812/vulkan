import global_resources;
import lighting_lib;

struct VSIn {
    float3 inPosition;
    float3 inNormal;
    float2 inTexCoord;
    float3 inTangent;
};

[shader("vertex")]
float4 mainVertex(VSIn vs_in, uniform float4x4 model, uniform int lightIndex)
    : SV_Position {
    return mul(lights[lightIndex].proj,
               mul(model, float4(vs_in.inPosition, 1.0f)));
}
