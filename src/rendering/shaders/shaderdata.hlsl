#include "bindlessresources.hlsli"
#include "common.hlsli"

[shader("raygeneration")]
void RayGenShader()
{
    RWTexture2D<float4> renderTarget = g_RWTextures[g_ResourceIndices.RenderTargetIndex];
    SceneConstants sceneConstants = g_Buffers[g_ResourceIndices.SceneBufferIndex].Load<SceneConstants>(0);
    
    Ray ray = GenerateCameraRay(sceneConstants.CameraPosition, sceneConstants.InvProjMatrix, sceneConstants.InvViewMatrix);
    float4 finalColor = TraceRadianceRay(ray, 0);
    
    renderTarget[DispatchRaysIndex().xy] = finalColor;
}

[shader("miss")]
void MissShader_Radiance(inout RayPayload payload)
{
    TextureCube environmentMap = g_CubeMaps[g_ResourceIndices.EnvironmentMapIndex];
    payload.Color = environmentMap.SampleLevel(g_LinearWrapSampler, WorldRayDirection(), 0);
}

[shader("miss")]
void MissShader_Shadow(inout ShadowRayPayload payload)
{
    payload.Hit = false;
}

#include "lambertshading.hlsli"
#include "phongshading.hlsli"
#include "pbrshading.hlsli"