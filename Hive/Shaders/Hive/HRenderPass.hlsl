struct HPSInput
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
    uint TextureIndex : SV_InstanceID;
};

Texture2D TextureArray[] : register(t0, space1);
SamplerState Sampler : register(s0);

struct HSceneBuffer
{
    float4 Translation;
    float4 Scale;
    float4 Padding[14];
};

struct HInstanceBuffer
{
    // Pack TextureIndex into Translation.w
    float4 Translation;
};

HSceneBuffer SceneBuffer : register(b0);

StructuredBuffer<HInstanceBuffer> InstanceBuffers : register(t1);

HPSInput VSMain(float4 Position : POSITION, float4 UV : TEXCOORD, uint InstanceID : SV_InstanceID)
{
    HPSInput Result;

    Result.Position.xyz = (InstanceBuffers[InstanceID].Translation.xyz + Position.xyz + SceneBuffer.Translation.xyz) * SceneBuffer.Scale.xyz / 1024.0;
    Result.Position.w = 1.0;
    Result.UV = UV.xy;
    Result.TextureIndex = asuint(InstanceBuffers[InstanceID].Translation.w);

    return Result;
}

float4 PSMain(HPSInput Input) : SV_TARGET
{
    return float4(Input.UV, 0.0, 1.0);
    return TextureArray[Input.TextureIndex].Sample(Sampler, Input.UV);
}