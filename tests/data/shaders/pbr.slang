import global_resources;
import lighting_lib;

public struct VSIn {
    float3 inPosition;
    float3 inNormal;
    float2 inTexCoord;
    float3 inTangent;
}

public struct VSOut {
    float4 c_position : SV_Position;
    float3 w_n;
    float2 tex_coord;
    float3 w_pos;
    float3 w_bitangent;
    float3 w_tangent;
}

struct MatParms {
    float ambient_i = 0.01;
    int tiling = 1;
    float specular = 0.0;
    Texture2D albedo;
    Texture2D normal;
};

// here you can add as many as you want
[vk_binding(0, 1)]
ConstantBuffer<MatParms> mat;

[shader("vertex")]
VSOut mainVertex(VSIn vs_in, uniform float4x4 model) {
    float3 N = normalize(mul(model, float4(vs_in.inNormal, 0.0f)).xyz);
    float3 T = normalize(mul(model, float4(vs_in.inTangent, 0.0f)).xyz);

    T = normalize(T - dot(T, N) * N);
    float3 B = normalize(cross(N, T));

    VSOut vs_out;
    vs_out.w_bitangent = B;

    vs_out.w_pos = mul(model, float4(vs_in.inPosition, 1.0f)).xyz;
    vs_out.tex_coord = vs_in.inTexCoord;
    vs_out.tex_coord.y = 1.0 - vs_out.tex_coord.y;
    vs_out.w_tangent = T;
    vs_out.w_n = N;

    vs_out.c_position = mul(
        ubo.proj, mul(ubo.view, mul(model, float4(vs_in.inPosition, 1.0f))));
    return vs_out;
}

[shader("pixel")]
float4 mainPixel(VSOut fs_in) : SV_Target0 {
    float3 albedo =
        mat.albedo.Sample(_sampler, fs_in.tex_coord * mat.tiling).rgb;
    float3 n = mat.normal.Sample(_sampler, fs_in.tex_coord * mat.tiling).xyz;
    // gl flip y
    // n.y = 1-n.y;
    n = (n * 2.) - 1.;
    float3 lcolor = albedo * mat.ambient_i;

    float3x3 tbn =
        transpose(float3x3(normalize(fs_in.w_tangent),
                           normalize(fs_in.w_bitangent), normalize(fs_in.w_n)));

    float3 w_n = normalize(mul(tbn, n));
    // float3 w_n = normalize(fs_in.w_n);

    for (int i = 0; i < ubo.nLights; i++) {
        Light light = lights[i];
        float3 v = normalize(light.position - ubo.obs);
        if (light.type == 0) {
            float3 w_l = normalize(light.position - fs_in.w_pos);
            // public float spec(float3 L, float3 N, float3 V, int exp) {
            lcolor += (diffuse(w_l, w_n, albedo) +
                       spec(w_l, w_n, v, 64, float3(1.0f)).rgb) *
                      spotLight(light, fs_in.w_pos, w_n, 5);
        } else if (light.type == 1) {
            float3 w_l = normalize(-light.direction);
            lcolor += (diffuse(w_l, w_n, albedo) +
                       spec(w_l, w_n, v, 64, float3(1.0f)).rgb) *
                      dirLight(light, i, fs_in.w_pos, w_n, 5);
            i += 3;
        }
    }

    return float4(lcolor, 1.0f);
}
