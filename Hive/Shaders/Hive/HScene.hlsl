// Define the root signature used by the shader
// ...

struct HRenderedSphere
{
	float4 SphereCenter;
	float4 RayOriginToSphereCenter;
	float RadiusSquared;
	uint MaterialIndex;
};

struct HRenderedScene
{
	float3 RayOrigin;
	uint SphereCount;
	uint SDFCount;
};

struct HMaterial
{
	float3 Albedo;
	float Roughness;
	float Metallic;
};

struct HRadianceField
{
	float3x4 A0;
};

struct HSDF
{
	float4 PositionRadius;
};

// Define the input and output resources used by the shader
RWTexture2D<float4> OutputTexture : register(u0);
RWStructuredBuffer<HRenderedSphere> RenderedSpheres : register(u1);
RWStructuredBuffer<HMaterial> Materials : register(u2);
RWStructuredBuffer<HRenderedScene> RenderedScenes : register(u3);
RWStructuredBuffer<HSDF> SDFs : register(u4);