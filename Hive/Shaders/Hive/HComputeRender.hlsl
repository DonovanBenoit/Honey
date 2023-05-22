// Define the root signature used by the shader
// ...

#include "HScene.hlsl"

uint EncodeDistance(float Distance)
{
	return uint(max(Distance * 1000.0, 0.0));
}

float DecodeDistance(uint EncodedDistance)
{
	return float(EncodedDistance) / 1000.0;
}

[numthreads(1, 1, 1)]
void clear(uint3 GroupID : SV_GroupID)
{
	OutputTexture[GroupID.xy] = float4(0.36, 0.69, 1.0, 1.0);
	MarchDistanceTexture[GroupID.xy] = 0;
	StepDistanceTexture[GroupID.xy] = EncodeDistance(1000.0);
}

[numthreads(1, 1, 1)]
void ApplyAndClearSphereDistance(uint3 GroupID : SV_GroupID)
{
	MarchDistanceTexture[GroupID.xy] = EncodeDistance(DecodeDistance(MarchDistanceTexture[GroupID.xy]) + DecodeDistance(StepDistanceTexture[GroupID.xy]));
	StepDistanceTexture[GroupID.xy] = EncodeDistance(1000.0);
}

[numthreads(64, 1, 1)]
void GetSphereDistance(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID)
{
	HRenderedScene RenderedScene = RenderedScenes[0];
	if (GroupThreadID.x >= RenderedScene.SDFCount)
	{
		return;
	}

	float3 RayOrigin = RenderedScene.RayOrigin;
	float3 RayDirection = normalize(float3((float2(GroupID.xy) - float2(512, 512)) / 1024.0, 1.0));
	float3 Position = RayOrigin + RayDirection * DecodeDistance(MarchDistanceTexture[GroupID.xy]);

	float4 PositionRadius = SDFs[GroupThreadID.x].PositionRadius;
	float Distance = length(Position - PositionRadius.xyz) - PositionRadius.a;

	InterlockedMin(StepDistanceTexture[GroupID.xy], EncodeDistance(Distance));
}

[numthreads(1, 1, 1)]
void ApplySphereDistance(uint3 GroupID : SV_GroupID)
{
	MarchDistanceTexture[GroupID.xy] = EncodeDistance(DecodeDistance(MarchDistanceTexture[GroupID.xy]) + DecodeDistance(StepDistanceTexture[GroupID.xy]));
}

// Define the compute shader entry point
[numthreads(1, 1, 1)]
void main(uint3 GroupID : SV_GroupID)
{

	// HRenderedScene RenderedScene = RenderedScenes[0];
	// Ray Generation
	//float3 RayOrigin = RenderedScene.RayOrigin;
	//float3 RayDirection = normalize(float3((float2(PixelCoord) - float2(512, 512)) / 1024.0, 1.0));

	float StepDistance = DecodeDistance(StepDistanceTexture[GroupID.xy]);
	if (StepDistance < 0.01)
	{
		float Distance = DecodeDistance(MarchDistanceTexture[GroupID.xy]);
		OutputTexture[GroupID.xy] = float4(Distance * 0.01, Distance * 0.5, Distance * 0.5, 1.0);
	}

	
	// Closest Hit
	/*float THit = 100000.0;
	uint HitSphere = 0xFFFFFFFF;
	for (uint Sphere = 0; Sphere < RenderedScene.SphereCount; Sphere++)
	{
		HRenderedSphere RenderedSphere = RenderedSpheres[Sphere];
		float SpherHitDistance = 0.0;
		if (RaySphereIntersect(RenderedSphere, RayDirection, SpherHitDistance))
		{
			if (SpherHitDistance < THit)
			{
				THit = SpherHitDistance;
				HitSphere = Sphere;
			}
		}
	}

	// Shading
	float3 LightDirection = float3(0.0, 0.0, 1.0);
	float3 Color = float3(0.16, 0.32, 0.64);
	if (HitSphere != 0xFFFFFFFF)
	{
		HRenderedSphere RenderedSphere = RenderedSpheres[HitSphere];
		float3 HitPosition = RayOrigin + RayDirection * THit;
		float3 HitNormal = normalize(HitPosition - RenderedSphere.SphereCenter.xyz);
		uint MaterialIndex = RenderedSphere.MaterialIndex;
		HMaterial Material = Materials[MaterialIndex];
		Color = dot(LightDirection, abs(HitNormal)) * Material.Albedo;
	}
	
	// Clear the output texture to solid blue
    OutputTexture[PixelCoord] = float4(Color, 1.0);*/
}