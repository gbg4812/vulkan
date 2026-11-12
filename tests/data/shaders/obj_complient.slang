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
    Texture2D diffuse_map;
    Texture2D bump_map;
    Texture2D env_map;
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

    float3 albedo = float3(1.0f) * mat.diffuse;
    if(mat.diffuse_map_b) {
        albedo *= mat.diffuse_map.Sample(_sampler, fs_in.tex_coord).rgb;
    }
    float4 lcolor = float4(0.f);
    float3 w_n = normalize(fs_in.w_n);
    if(mat.bump_map_b) {
        float3 normal = mat.bump_map.Sample(_sampler, fs_in.tex_coord).xyz;
        normal = (normal * 2.) - 1.;
        float3x3 tbn =
            transpose(float3x3(normalize(fs_in.w_tangent),
                               normalize(fs_in.w_bitangent), normalize(fs_in.w_n)));
    
        w_n = normalize(mul(tbn, normal));
    }
    
    float3 w_v = normalize(ubo.obs - fs_in.w_pos);
    float3 ref_col = mat.env_map.Sample(_sampler, reflect(w_v, w_n).xy).rgb;


    for (int i = 0; i < ubo.nLights; i++) {
        Light light = lights[i];
        if (light.type == LightType::SPOT) {
            float3 w_l = normalize(light.position - fs_in.w_pos);
            float3 diffuse_light = diffuse(w_l, w_n, albedo) *
                                  spotLight(light, fs_in.w_pos, w_n, 5);
            lcolor += float4(diffuse_light, 1.0f);
        } else if (light.type == LightType::DIRECTIONAL) {
            //public float spec(float3 L, float3 N, float3 V, int exp) {
            float3 light_i = dirLight(light, i, fs_in.w_pos, w_n, 5);
            float3 diffuse_light = diffuse(normalize(-light.direction), w_n, albedo) + ref_col;
            float3 spec_light = spec(normalize(-light.direction), w_n, w_v, int(mat.exponent), mat.specular).rbg;
            lcolor += float4(diffuse_light + spec_light, 1.0f);
            i += 3;
        } else if(light.type == LightType::AMBIENT) {
            lcolor += float4(ambient(light, albedo), 1.0f);
        }
    }

    return lcolor;
}
