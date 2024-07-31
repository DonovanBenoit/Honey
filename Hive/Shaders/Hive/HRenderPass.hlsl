struct HPSInput
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
};

// Texture2D g_texture : register(t0);
// SamplerState g_sampler : register(s0);

HPSInput VSMain(float4 Position : POSITION, float4 UV : TEXCOORD)
{
    HPSInput Result;

    Result.Position = Position;
    Result.UV = UV;

    return Result;
}

float4 PSMain(HPSInput Input) : SV_TARGET
{
    return float4(Input.UV, 0.4, 1.0);
}