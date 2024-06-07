#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

RayDesc GenerateCameraRay(in float3 cameraPosition, in float4x4 invProjMatrix, in float4x4 invViewMatrix)
{
    float2 screenPos = (float2) DispatchRaysIndex().xy / (float2) DispatchRaysDimensions().xy * 2.0 - 1.0; // Clip Space [-1, 1]
    screenPos.y = -screenPos.y;
    
    float4 positionVS = mul(invProjMatrix, float4(screenPos, 0.0, 1.0)); // To View Space
    float3 rayDirectionWS = mul(invViewMatrix, float4(normalize(positionVS.xyz / positionVS.w), 0.0)).xyz; // To World Space

    RayDesc ray;
    ray.Origin = cameraPosition;
    ray.Direction = rayDirectionWS;
    ray.TMin = 0.001;
    ray.TMax = 1000.0;
    
    return ray;
}

#endif // __COMMON_HLSLI__