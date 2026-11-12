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
    float3 color = { 0.1, 0.4, 0.3 };
    float3 spec = {1, 1, 1};
    int exp = 64;
    float opacity = 0.2;
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
    float4 lcolor = float4(0.0f);

    float3 w_n = normalize(fs_in.w_n);

    for (int i = 0; i < ubo.nLights; i++) {
        Light light = lights[i];
        if (light.type == 0) {
            float3 w_l = normalize(light.position - fs_in.w_pos);
            lcolor += float4(diffuse(w_l, w_n, mat.color) *
                      spotLight(light, fs_in.w_pos, w_n, 5), mat.opacity);
        } else if (light.type == 1) {
            float3 w_v = normalize(ubo.obs - fs_in.w_pos);
            float4 specular_light = spec(normalize(-light.direction), w_n, w_v, mat.exp, mat.spec);
            float3 diffuse_light = mat.color *
                                  dirLight(light, i, fs_in.w_pos, w_n, 5);
            lcolor += float4(diffuse_light, specular_light.a) + specular_light;
            i += 3;
        }
    }

    return lcolor;
}
