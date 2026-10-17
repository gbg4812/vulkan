
import global_resources;
import lighting_lib;

public struct VSIn {
    [[vk::location(0)]]
    float3 inPosition;
    [[vk::location(1)]]
    float3 inNormal;
    [[vk::location(2)]]
    float2 inTexCoord;
    [[vk::location(3)]]
    float3 inTangent;
}

public struct VSOut {
    float4 c_position : SV_Position;
    float3 w_n;
    float2 tex_coord;
    float3 w_pos;
    float3x3 w_TBN;
    float3 w_tangent;
}

struct MatParms {
    float3 color;
    float ambient_i = 0.01;
    float max_width = 5;
}

// here you can add as many as you want
[[vk_binding(1, 0)]]
ConstantBuffer<MatParms> mat;

// here you declare the textures you need
//[[vk_binding(1, 1)]]
// Texture2D _texture[2];

[[shader("vertex")]]
VSOut mainVertex(VSIn vs_in, uniform float4x4 model) : VSOUT {
    float3 N = normalize(mul(model, float4(vs_in.inNormal, 0.0f)).xyz);
    float3 T = normalize(mul(model, float4(vs_in.inTangent, 0.0f)).xyz);

    T = normalize(T - dot(T, N) * N);
    float3 B = normalize(cross(N, T));

    VSOut vs_out;
    vs_out.w_TBN = float3x3(T, B, N);

    vs_out.w_pos = mul(model, float4(vs_in.inPosition, 1.0f)).xyz;
    vs_out.tex_coord = vs_in.inTexCoord;
    vs_out.tex_coord.y = 1.0 - vs_out.tex_coord.y;
    vs_out.w_tangent = T;
    vs_out.w_n = N;

    vs_out.c_position = mul(ubo.proj, mul(ubo.view, mul(model, float4(vs_in.inPosition, 1.0f))));
    return vs_out;
}

[[shader("pixel")]]
float4 mainPixel(VSOut fs_in: VSOUT) : SV_Target0 {
    float3 lcolor = mat.color * mat.ambient_i;

    float3 w_n = normalize(fs_in.w_n);

    for (int i = 0; i < ubo.nLights; i++) {
        Light light = lights[i];
        if (light.type == 0) {
            float3 w_l = normalize(light.position - fs_in.w_pos);
            lcolor += mat.color * diffuse(w_l, w_n) *
                      spotLight(light, fs_in.w_pos, w_n, mat.max_width);
        } else if (light.type == 1) {
            lcolor += mat.color * diffuse(normalize(-light.direction), w_n) *
                      dirLight(light, i, fs_in.w_pos, w_n, mat.max_width);
            i += 3;
        }
    }

    return float4(lcolor, 1.0f);
}
