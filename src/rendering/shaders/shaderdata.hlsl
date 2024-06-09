#include "bindlessresources.hlsli"

#define DIRECTIONAL_LIGHT 0
#define POINT_LIGHT       1
#define SPOT_LIGHT        2
#define AREA_LIGHT        3 // todo

struct RayPayload
{
    float4 Color;
};

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


[shader("raygeneration")]
void RayGenShader()
{
    RWTexture2D<float4> renderTarget = g_RWTextures[g_ResourceIndices.RenderTargetIndex];
    RaytracingAccelerationStructure accelerationStructure = g_AccelerationStructures[g_ResourceIndices.AccelerationStructureIndex];
    SceneConstants sceneConstants = g_Buffers[g_ResourceIndices.SceneBufferIndex].Load < SceneConstants > (0);
    
    RayDesc ray = GenerateCameraRay(sceneConstants.CameraPosition, sceneConstants.InvProjMatrix, sceneConstants.InvViewMatrix);
    RayPayload payload = { float4(0, 0, 0, 0) };
    
    TraceRay(accelerationStructure, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 0, 0, ray, payload);

    renderTarget[DispatchRaysIndex().xy] = payload.Color;
}

[shader("miss")]
void MissShader(inout RayPayload payload)
{
    TextureCube environmentMap = g_CubeMaps[g_ResourceIndices.EnvironmentMapIndex];
    payload.Color = environmentMap.SampleLevel(g_LinearWrapSampler, WorldRayDirection(), 0);
}

#include "lambertshading.hlsli"
#include "phongshading.hlsli"