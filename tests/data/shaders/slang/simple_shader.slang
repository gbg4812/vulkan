import global_resources;

struct MatParms {
    float3 color = { 1., 1., 1. };
}

struct VSIn {
    float3 inPosition;
    float3 inNormal;
    float2 inTexCoord;
    float3 inTangent;
};

struct VSOut {
    float4 position_c : SV_Position;
};

[vk_binding(0, 1)]
ConstantBuffer<MatParms> mat;

[shader("pixel")]
float4 mainPixel(VSOut fg_in) : SV_Target0 {
    return float4(mat.color, 1.0f);
}

[shader("vertex")]
VSOut mainVertex(VSIn vs_in, uniform float4x4 model) {
    VSOut out;
    out.position_c = mul(
        ubo.proj, mul(ubo.view, mul(model, float4(vs_in.inPosition, 1.0f))));
    return out;
}
