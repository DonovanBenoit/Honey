// Define the root signature used by the shader
// ...

#include "HScene.hlsl"

void ComputeSphereDistanceA(float3 RayOrigin, float3 RayDirection, inout float StepDistance, inout float3 HitPosition, inout float3 HitNormal)
{
	float3 Position = RayOrigin;

	for (int Step = 0; Step < 32; Step++)
	{
		float Distance = 1000.0;
		uint ClosestSDFIndex = 0;
		for (uint SDFIndex = 0; SDFIndex < 32; SDFIndex++)
		{
			float4 PositionRadius = SDFs[SDFIndex].PositionRadius;
			float SDFDistance = length(Position - PositionRadius.xyz) - PositionRadius.a;
			if (SDFDistance < Distance)
			{
				Distance = SDFDistance;
				ClosestSDFIndex = SDFIndex;
			}
		}

		Position += RayDirection * Distance;
		StepDistance = Distance;

		if (Distance < 0.1)
		{
			HitPosition = Position;

			// Cheating
			float4 PositionRadius = SDFs[ClosestSDFIndex].PositionRadius;
			HitNormal = normalize(HitPosition - PositionRadius.xyz);
			return;
		}
	}
}

void ComputeSphereDistanceB(float3 RayOrigin, float3 RayDirection, inout float StepDistance, inout float3 HitPosition, inout float3 HitNormal)
{
	StepDistance = 1000.0;

	float MinMarchDistance = 1000.0;
	for (uint SDFIndex = 0; SDFIndex < 32; SDFIndex++)
	{
		float4 PositionRadius = SDFs[SDFIndex].PositionRadius;
		float3 Position = RayOrigin;
		float MarchDistance = 0.0;

		for (int Step = 0; Step < 32; Step++)
		{
			float Distance = length(Position - PositionRadius.xyz) - PositionRadius.a;
			MarchDistance += Distance;
			Position += RayDirection * Distance;
			if (Distance < 0.1)
			{
				if (MarchDistance < MinMarchDistance)
				{
					MinMarchDistance = MarchDistance;
					StepDistance = Distance;
					HitPosition = Position;

					float RightDistance = length(Position + float3(0.05, 0.0, 0.0) - PositionRadius.xyz) - PositionRadius.a;
					float DownDistance = length(Position + float3(0.0, 0.05, 0.0) - PositionRadius.xyz) - PositionRadius.a;

					HitNormal = normalize(float3(RightDistance, DownDistance, 0.1));
				}
				break;
			}
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
	float3 HitPosition = float3(0.0, 0.0, 0.0);
	float3 HitNormal = float3(0.0, 0.0, 0.0);
	ComputeSphereDistanceA(RenderedScene.RayOrigin, RayDirection, StepDistance, HitPosition, HitNormal);

	if (StepDistance < 0.1)
	{
		OutputTexture[GroupID.xy] = float4(abs(HitNormal), 1.0);
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