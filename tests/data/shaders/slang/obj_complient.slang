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
    float3 diffuse = { 0.1, 0.4, 0.3 };
    float3 ambient = { 0.2, 0.2, 0.2 };
    float3 specular = { 1, 1, 1 };
    float exponent = 64;
    bool diffuse_map_b = false;
    bool bump_map_b = false;
    float opacity = 1.0f;
    Texture2D diffuse_map;
    Texture2D bump_map;
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

    float3 albedo = float3(1.0f);
    if(mat.diffuse_map_b) {
        albedo = mat.diffuse_map.Sample(_sampler, fs_in.tex_coord).rgb;
    }
    float4 lcolor = float4(mat.ambient * albedo, mat.opacity);
    float3 w_n = normalize(fs_in.w_n);
    if(mat.bump_map_b) {
        float3 normal = mat.bump_map.Sample(_sampler, fs_in.tex_coord).xyz;
        normal = (normal * 2.) - 1.;
        float3x3 tbn =
            transpose(float3x3(normalize(fs_in.w_tangent),
                               normalize(fs_in.w_bitangent), normalize(fs_in.w_n)));
    
        w_n = normalize(mul(tbn, normal));
    }


    for (int i = 0; i < ubo.nLights; i++) {
        Light light = lights[i];
        if (light.type == 0) {
            float3 w_l = normalize(light.position - fs_in.w_pos);
            lcolor += float4(albedo * mat.diffuse * diffuse(w_l, w_n) *
                      spotLight(light, fs_in.w_pos, w_n, 5), 1.0f);
        } else if (light.type == 1) {
            float3 w_v = normalize(ubo.obs - fs_in.w_pos);
            //public float spec(float3 L, float3 N, float3 V, int exp) {
            float3 light_i = dirLight(light, i, fs_in.w_pos, w_n, 5);
            lcolor += float4(mat.diffuse * albedo * light_i * diffuse(normalize(-light.direction), w_n), mat.opacity);
            float spec = spec(normalize(-light.direction), w_n, w_v, int(mat.exponent));
            lcolor += float4(mat.specular * spec, spec);
            i += 3;
        }
    }

    return lcolor;
}
