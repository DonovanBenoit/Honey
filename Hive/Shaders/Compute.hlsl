RWTexture2D<float4> OutputBuffer : register(u0);


cbuffer DSceneBuffer : register(b0)
{
	float4x4 CameraRotation;
	float4 CameraOrigin;
	float CameraFocalLength;
	uint ShapeCount;
};

#define Shape_Sphere 0

struct DShape
{
    float4 PositionRadius;
	uint ShapeType;
};
StructuredBuffer<DShape> ShapeBuffer : register(t0);

float SDSphere(in float3 Displacement, in float Radius)
{
  return length(Displacement) - Radius;
}

bool IntersectSphere(in float3 RayOrigin, in float3 RayDirection, in float3 Center, in float Radius, inout float t1, inout float t2)
{
    float3 L = Center - RayOrigin;
    float tca = dot(L, RayDirection);
    float d2 = dot(L, L) - tca * tca;

    if (d2 > Radius * Radius) {
        return false;
    }

    float thc = sqrt(Radius * Radius - d2);
    t1 = tca - thc;
    t2 = tca + thc;

    return true;
}

bool IntersectAABB(in float3 RayOrigin, in float3 RayDirection, in float3 Min, in float3 Max, inout float TNear, inout float TFar)
{
    float t1 = (Min.x - RayOrigin.x) / RayDirection.x;
    float t2 = (Max.x - RayOrigin.x) / RayDirection.x;
    float t3 = (Min.y - RayOrigin.y) / RayDirection.y;
    float t4 = (Max.y - RayOrigin.y) / RayDirection.y;
    float t5 = (Min.z - RayOrigin.z) / RayDirection.z;
    float t6 = (Max.z - RayOrigin.z) / RayDirection.z;

    TNear = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
    TFar = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

    return TNear < TFar && TFar > 0.0f;
}

void GenerateRay(in float2 ScreenUV, out float3 RayOrigin, out float3 RayDirection)
{
	RayOrigin = CameraOrigin.xyz;
	
	float3 ViewDirection;
	ViewDirection.z = CameraFocalLength;
	ViewDirection.xy = 2.0 * (ScreenUV - 0.5);
	
	RayDirection = normalize(mul(float4(normalize(ViewDirection), 0.0), CameraRotation).xyz);
}

float SDScene(in float3 Position)
{
	float3 SphereOrigin = float3(0.0, 0.0, 10.0);
	return SDSphere(Position - SphereOrigin, 2.0);
}

bool TraceRay(in float3 RayOrigin, in float3 RayDirection, inout float3 Position)
{
	bool IntersectionFound = false;
	float TCurrent = 10000.0;
	
	for (uint ShapeIndex = 0; ShapeIndex < ShapeCount; ShapeIndex++)
	{
		DShape Shape = ShapeBuffer[ShapeIndex];
		float T1, T2;
		if (IntersectSphere(RayOrigin, RayDirection, Shape.PositionRadius.xyz, Shape.PositionRadius.w, T1, T2))
		{
			float TSphere = min(max(T1, 0.0), max(T1, 0.0));
			if (TSphere < TCurrent)
			{
				TCurrent = TSphere;
				IntersectionFound = true;
			}
		}
	}
	
	Position = RayOrigin + RayDirection * TCurrent;
	return IntersectionFound;
}

[numthreads(8, 8, 1)] // Define thread group size (64 threads in 1D)
void CSMain(uint3 GroupID : SV_GroupID, uint3 DispatchThreadID : SV_DispatchThreadID, uint3 GroupThreadID : SV_GroupThreadID, uint GroupIndex : SV_GroupIndex)
{
	float2 ScreenUV = float2(DispatchThreadID.xy) / 1024.0;
	
	float3 RayOrigin;
	float3 RayDirection;
	GenerateRay(ScreenUV, RayOrigin, RayDirection);
	
	float3 Position;
	if (TraceRay(RayOrigin, RayDirection, Position))
	{
		OutputBuffer[DispatchThreadID.xy] = float4(abs(fmod(Position, 1.0)), 1.0);
	}
	else
	{
		OutputBuffer[DispatchThreadID.xy] = float4(abs(RayDirection), 1.0);
	}
}