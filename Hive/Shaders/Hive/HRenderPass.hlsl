struct HPSInput
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
};

Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

struct HSceneBuffer
{
    float4 Translation;
    float4 Scale;
    float4 Padding[14];
};

struct HInstanceBuffer
{
    float4 Translation;
    float4 Padding[15];
};

HSceneBuffer SceneBuffer : register(b0);

HInstanceBuffer InstanceBuffer : register(b1);

HPSInput VSMain(float4 Position : POSITION, float4 UV : TEXCOORD, uint InstanceID : SV_InstanceID)
{
    HPSInput Result;

    Result.Position.xyz = (InstanceBuffer.Translation.xyz + Position.xyz + float3(InstanceID * 40.0, 0.0, 0.0)) * SceneBuffer.Scale / 1024.0;
    Result.Position.w = 1.0;
    Result.UV = UV;

    return Result;
}

float4 PSMain(HPSInput Input) : SV_TARGET
{
    return Texture.Sample(Sampler, Input.UV);
}