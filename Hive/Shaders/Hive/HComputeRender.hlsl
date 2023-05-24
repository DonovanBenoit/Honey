// Define the root signature used by the shader
// ...

#include "HScene.hlsl"

void ComputeSphereDistance(float3 RayOrigin, float3 RayDirection, inout float StepDistance)
{
	float3 Position = RayOrigin;

	for (int i = 0; i < 32; i++)
	{
		float Distance = 1000.0;
		for (uint SDFIndex = 0; SDFIndex < 32; SDFIndex++)
		{
			float4 PositionRadius = SDFs[SDFIndex].PositionRadius;
			Distance = min(length(Position - PositionRadius.xyz) - PositionRadius.a, Distance);
		}

		Position += RayDirection * Distance;
		StepDistance = Distance;

		if (Distance < 0.1)
		{
			return;
		}
	}
}

// Define the compute shader entry point
[numthreads(1, 1, 1)]
void main(uint3 GroupID : SV_GroupID)
{
	HRenderedScene RenderedScene = RenderedScenes[0];
	float3 RayDirection = normalize(float3((float2(GroupID.xy) - float2(512, 512)) / 1024.0, 1.0));

	float StepDistance = 0.0;
	ComputeSphereDistance(RenderedScene.RayOrigin, RayDirection, StepDistance);

	if (StepDistance < 0.1)
	{
		OutputTexture[GroupID.xy] = float4(1.0, 0.0, 0.0, 1.0);
	}
	else
	{
		OutputTexture[GroupID.xy] = float4(0.36, 0.69, 1.0, 1.0);
	}

	//MarchDistanceTexture[GroupID.xy] = 0;
	//StepDistanceTexture[GroupID.xy] = EncodeDistance(1000.0);

	
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