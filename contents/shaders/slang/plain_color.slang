import global_resources;

struct MatParms {
    float3 color = { 1., 1., 1. };
}

[vk::binding(0, 1)]
ConstantBuffer<MatParms> mat;

[shader("pixel")]
float4 mainPixel() : SV_Target0 {
    return float4(mat.color, 1.0f);
}

struct VSIn {
    float3 inPosition;
    float3 inNormal;
    float2 inTexCoord;
    float3 inTangent;
};

[[shader("vertex")]]
float4 mainVertex(VSIn vs_in, uniform float4x4 model) : SV_Position {
    return mul(ubo.proj,
               mul(ubo.view, mul(model, float4(vs_in.inPosition, 1.0f))));
}
