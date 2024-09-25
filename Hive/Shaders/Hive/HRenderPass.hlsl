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
};

HSceneBuffer SceneBuffer : register(b0);

StructuredBuffer<HInstanceBuffer> InstanceBuffers : register(t1);

HPSInput VSMain(float4 Position : POSITION, float4 UV : TEXCOORD, uint InstanceID : SV_InstanceID)
{
    HPSInput Result;

    Result.Position.xyz = (InstanceBuffers[InstanceID].Translation.xyz + Position.xyz) * SceneBuffer.Scale / 1024.0;
    Result.Position.w = 1.0;
    Result.UV = UV;

    return Result;
}

float4 PSMain(HPSInput Input) : SV_TARGET
{
    return Texture.Sample(Sampler, Input.UV);
}