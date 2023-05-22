// Define the root signature used by the shader
// ...

#include "HScene.hlsl"

bool RayRadianceFieldIntersect(HRadianceField RadianceField, float3 RayDirection, inout float IntersectionDistance)
{
	return false;
};

float SampleSDF(float3 Position, HRenderedScene RenderedScene)
{
	float Distance = 1000.0;
	for (uint SDFIndex = 0; SDFIndex < RenderedScene.SDFCount; SDFIndex++)
	{
		float4 PositionRadius = SDFs[SDFIndex].PositionRadius;
		Distance = min(length(Position - PositionRadius.xyz) - PositionRadius.a, Distance);
	}
	return Distance;
}

bool RayMarch(float3 RayOrigin, float3 RayDirection, HRenderedScene RenderedScene)
{
	float3 Position = RayOrigin;

	float Distance = SampleSDF(Position, RenderedScene);

	for (int Step = 0; Step < 16; Step++)
	{
		Distance = SampleSDF(Position, RenderedScene);
		Position += RayDirection * Distance;

		if (Distance < 0.1)
		{
			return true;
		}
	}

	return false;
}

bool RaySphereIntersect(HRenderedSphere RenderedSphere, float3 RayDirection, inout float IntersectionDistance)
{
	float t0 = dot(RenderedSphere.RayOriginToSphereCenter.xyz, RayDirection);
	float d2 = dot(RenderedSphere.RayOriginToSphereCenter.xyz, RenderedSphere.RayOriginToSphereCenter.xyz) - t0 * t0;
	if (d2 > RenderedSphere.RadiusSquared)
	{
		// The ray misses the sphere
		return false;
	}

	float t1 = sqrt(RenderedSphere.RadiusSquared - d2);
	IntersectionDistance = t0 > (t1 + 0.000001) ? t0 - t1 : t0 + t1;
	return IntersectionDistance > 0.000001;
}

// Define the compute shader entry point
[numthreads(1, 1, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    // Compute the output pixel coordinates from the dispatch thread ID
    uint2 PixelCoord = DispatchThreadId.xy;

	HRenderedScene RenderedScene = RenderedScenes[0];
	
	// Ray Generation
	float3 RayOrigin = RenderedScene.RayOrigin;
	float3 RayDirection = normalize(float3((float2(PixelCoord) - float2(512, 512)) / 1024.0, 1.0));

	if (RayMarch(RayOrigin, RayDirection, RenderedScene))
	{
		OutputTexture[PixelCoord] = float4(1.0, 0.0, 0.0, 1.0);
	}
	else
	{
		OutputTexture[PixelCoord] = float4(0.0, 1.0, 0.0, 1.0);
	}
	return;

	
	// Closest Hit
	float THit = 100000.0;
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
    OutputTexture[PixelCoord] = float4(Color, 1.0);
}