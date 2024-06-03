#include "BindlessResources.hlsli"

struct RayPayload
{
    float4 Color;
};

[shader("raygeneration")]
void RayGenShader()
{
    RWTexture2D<float4> renderTarget = g_RWTextures[g_ResourceIndices.RenderTargetIndex];
    RaytracingAccelerationStructure accelerationStructure = g_AccelerationStructures[g_ResourceIndices.AccelerationStructureIndex];
    
    float2 lerpValues = (float2)DispatchRaysIndex() / (float2)DispatchRaysDimensions();

    RayDesc ray;
    ray.Origin = float3(lerp(-1.0, 1.0, lerpValues.x), lerp(-1.0, 1.0, lerpValues.y), 0.0f);
    ray.Direction = float3(0, 0, 1);
    ray.TMin = 0.001;
    ray.TMax = 1000.0;
    
    RayPayload payload = { float4(0, 0, 0, 0) };
    TraceRay(accelerationStructure, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);

    renderTarget[DispatchRaysIndex().xy] = payload.Color;
}

[shader("closesthit")]
void ClosestHitShader(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    payload.Color = float4(barycentrics, 1);
}

[shader("miss")]
void MissShader(inout RayPayload payload)
{
    payload.Color = float4(0.1, 0.2, 0.6, 1);
}