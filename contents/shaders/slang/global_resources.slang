module global_resources;

public struct PerFrameGlobalU {
    public float4x4 view;
    public float4x4 proj;
    public float3 obs;
    public float time;
    public int nLights;
}

[[vk_binding(0, 0)]]
public ConstantBuffer<PerFrameGlobalU> ubo;

[[vk_binding(0, 1)]]
public SamplerState _sampler;
