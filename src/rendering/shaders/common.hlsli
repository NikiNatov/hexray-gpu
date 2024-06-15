#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

#define DIRECTIONAL_LIGHT 0
#define POINT_LIGHT       1
#define SPOT_LIGHT        2
#define AREA_LIGHT        3 // todo

#define MAX_RAY_RECURSION_DEPTH 3

struct Ray
{
    float3 Origin;
    float3 Direction;
};

struct RayPayload
{
    float4 Color;
    uint RecursionDepth;
};

struct ShadowRayPayload
{
    bool Hit;
};

Ray GenerateCameraRay(in float3 cameraPosition, in float4x4 invProjMatrix, in float4x4 invViewMatrix)
{
    float2 screenPos = (float2) DispatchRaysIndex().xy / (float2) DispatchRaysDimensions().xy * 2.0 - 1.0; // Clip Space [-1, 1]
    screenPos.y = -screenPos.y;
    
    float4 positionVS = mul(invProjMatrix, float4(screenPos, 0.0, 1.0)); // To View Space
    float3 rayDirectionWS = mul(invViewMatrix, float4(normalize(positionVS.xyz / positionVS.w), 0.0)).xyz; // To World Space

    Ray ray;
    ray.Origin = cameraPosition;
    ray.Direction = rayDirectionWS;
    
    return ray;
}

float4 TraceRadianceRay(in Ray ray, in uint currentRayRecursionDepth)
{
    if (currentRayRecursionDepth >= MAX_RAY_RECURSION_DEPTH)
    {
        return float4(0, 0, 0, 0);
    }

    RayDesc rayDesc;
    rayDesc.Origin = ray.Origin;
    rayDesc.Direction = ray.Direction;
    rayDesc.TMin = 0.01;
    rayDesc.TMax = 1000;
    
    RaytracingAccelerationStructure accelerationStructure = g_AccelerationStructures[g_ResourceIndices.AccelerationStructureIndex];
    RayPayload payload = { float4(0, 0, 0, 0), currentRayRecursionDepth + 1 };
    
    TraceRay(accelerationStructure, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 0, 0, rayDesc, payload);

    return payload.Color;
}

bool TraceShadowRay(in Ray ray, in uint currentRayRecursionDepth)
{
    if (currentRayRecursionDepth >= MAX_RAY_RECURSION_DEPTH)
    {
        return false;
    }

    RayDesc rayDesc;
    rayDesc.Origin = ray.Origin;
    rayDesc.Direction = ray.Direction;
    rayDesc.TMin = 0.01;
    rayDesc.TMax = 1000;
    
    RaytracingAccelerationStructure accelerationStructure = g_AccelerationStructures[g_ResourceIndices.AccelerationStructureIndex];
    ShadowRayPayload payload = { true };
    
    TraceRay(accelerationStructure, RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, ~0, 0, 0, 1, rayDesc, payload);

    return payload.Hit;
}

float3 GetNormalFromMap(Texture2D<float4> normalMap, float2 texCoord, float3 tangentWS, float3 bitangentWS, float3 normalWS)
{
    float3x3 tgnMatrix = float3x3(normalize(tangentWS), normalize(bitangentWS), normalize(normalWS));
    float3 normalMapValue = normalMap.SampleLevel(g_LinearClampSampler, texCoord, 0).rgb * 2.0 - 1.0;
    return normalize(mul(normalMapValue, tgnMatrix));
}

#endif // __COMMON_HLSLI__