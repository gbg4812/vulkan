
import global_resources;

public struct Light {
    public float3 color;
    public float3 direction;
    public float3 position;
    public float4x4 proj;
    public float intensity;
    public int shadow_map;
    public int type;
};

[[vk_binding(0, 2)]]
public StructuredBuffer<Light> lights;

[[vk_binding(0, 2)]]
uniform DepthTexture2D _shadow_map;
uniform SamplerComparisonState _shadow_sampler;

float2 rotate2D(float2 vec, float angle) {
    float2x2 m = float2x2(cos(angle), -sin(angle), sin(angle), cos(angle));

    return mul(m, vec);
}

static const float2 sample_pattern[] = {
    float2(-0.94201624, -0.39906216),  float2(0.94558609, -0.76890725),
    float2(-0.094184101, -0.92938870), float2(0.34495938, 0.29387760),
    float2(-0.91588581, 0.45771432),   float2(-0.81544232, -0.87912464),
    float2(-0.38277543, 0.27676845),   float2(0.97484398, 0.75648379),
    float2(0.44323325, -0.97511554),   float2(0.53742981, -0.47373420),
    float2(-0.26496911, -0.41893023),  float2(0.79197514, 0.19090188),
    float2(-0.24188840, 0.99706507),   float2(-0.81409955, 0.91437590),
    float2(0.19984126, 0.78641367),    float2(0.14383161, -0.14100790)
};

public float computeSoftShadow(Light light, float3 w_pos, float4 cam_pos,
                        float max_width, float3 w_n) {
    float3 L = normalize(light.position - w_pos);
    float2 coords = cam_pos.xy;
    coords += 1.;
    coords /= 2.;
    coords.x /= 10;                             // alongated texture
    coords.x += light.shadow_map * (1. / 10.);  // move to the correct place
    float2 off = { (1. / (2048. * 10.)), (1. / 2048.) };

    float rand_a = sin(52253 * coords.x + 9709039 * coords.y) * 6.283185;
    float oc_d = 0;
    int oc_n = 0;
    for (int i = 0; i < sample_pattern.getCount(); i++) {
        float comp_value =
            cam_pos.z - (0.0005 + 0.0001 * length(sample_pattern[i]));
        float closest = _shadow_map.Sample(
            _sampler,
            coords + rotate2D(sample_pattern[i], rand_a) * off * max_width);
        if (closest < comp_value) {
            oc_d += closest;
            oc_n += 1;
        }
    }

    float shadow = 0;
    oc_d = oc_d / oc_n;
    float oc_f = clamp(max_width * 2 * (cam_pos.z - oc_d) / cam_pos.z, 0.1, 1);
    int nsamples = sample_pattern.getCount();
    for (int i = 0; i < nsamples; i++) {
        float comp_value =
            cam_pos.z - (0.0005 + 0.0001 * length(sample_pattern[i]));
        float value =
            _shadow_map.SampleCmp(_shadow_sampler,
                                  coords + rotate2D(sample_pattern[i], rand_a) *
                                               off * max_width * oc_f,
                                  comp_value);
        shadow += (1 - value) / nsamples;
    }

    return shadow;
}

public float computeShadow(Light light, float3 w_pos, float4 cam_pos, float max_width,
                    float3 w_n) {
    float3 L = normalize(light.position - w_pos);
    float2 coords = cam_pos.xy;
    coords += 1.;
    coords /= 2.;
    coords.x /= 10;                             // alongated texture
    coords.x += light.shadow_map * (1. / 10.);  // move to the correct place
    float2 off = { (1. / (2048. * 10)), (1. / 2048.) };

    float shadow = 0;
    int kernel_width = 7;
    for (int i = 0; i < kernel_width; i++) {
        for (int j = 0; j < kernel_width; j++) {
            float2 sampl = (float2(i, j) - kernel_width / 2);
            float closest = _shadow_map.SampleCmp(
                _shadow_sampler, coords + sampl * off,
                cam_pos.z - (0.0005 + length(sampl) * 0.001 +
                             max(0, dot(w_n, L)) * 0.001));
            shadow += (1 - closest) / pow(kernel_width, 2);
        }
    }

    return shadow;
}

public float3 spotLight(Light light, float3 w_pos, float3 w_n, float max_width) {
    // compute position from lights perspective
    float3 L = light.position - w_pos;
    float4 cam_pos = mul(light.proj, float4(w_pos, 1.0f));
    cam_pos /= cam_pos.w;
    float dist = length(L);
    // spot light mask and decay
    float bright = smoothstep(0, 0.1, 1 - length(cam_pos.xy)) *
                   (1.0 / (dist * dist)) * light.intensity *
                   max(dot(L, -light.direction), 0);

    if (light.shadow_map > -1 && bright > 0)
        bright *= computeShadow(light, w_pos, cam_pos, max_width, w_n);

    return light.color * bright;
}

public float3 dirLight(Light light, int i, float3 w_pos, float3 w_n, float max_width) {
    float4 v_pos = mul(ubo.view, float4(w_pos, 1.0f));
    
    // light.position in directional lights means (znear, zfar, _)
    float z_p =
        ((-v_pos.z) - light.position.x) / (light.position.y - light.position.x);
    int l_idx = 0;
    if (z_p > 1.f / 8.f) {
        l_idx = 4 - int(ceil(log2(1.0f / z_p)));
    }
    Light slight = lights[i + l_idx];
    // compute position from lights perspective
    float4 cam_pos = mul(slight.proj, float4(w_pos, 1.0f));
    cam_pos /= cam_pos.w;
    float bright = slight.intensity;

    if (slight.shadow_map > -1 && bright > 0)
        bright *= computeShadow(slight, w_pos, cam_pos, max_width, w_n);

    // return float3(l_idx == 0, l_idx == 1 || l_idx == 3, l_idx == 2 || l_idx
    // == 3);
    return slight.color * bright;
}

public float diffuse(float3 L, float3 N) {
    float diff = dot(L, normalize(N));
    diff = max(0., diff);
    return diff;
}

public float spec(float3 L, float3 N, float3 V, int exp) {
    float3 R = normalize(reflect(-L, N));
    float VdotR = max(0, dot(V, R));
    return pow(VdotR, exp);
}
