#ifndef HLSL
#define HLSL
#endif

#include "resources.h"
#include "random.hlsli"
#include "lambertshading.hlsli"
#include "phongshading.hlsli"
#include "pbrshading.hlsli"

[shader("raygeneration")]
void RayGenShader()
{
    uint2 rayID = DispatchRaysIndex().xy;
    uint2 rayDimensions = DispatchRaysDimensions().xy;
    
    RWTexture2D<float4> renderTarget = g_RWTextures[g_ResourceIndices.RenderTargetIndex];
    RWTexture2D<float4> prevFrameRenderTarget = g_RWTextures[g_ResourceIndices.PrevFrameRenderTargetIndex];
    
    SceneConstants sceneConstants = g_Buffers[g_ResourceIndices.SceneBufferIndex].Load<SceneConstants>(0);
    RaytracingAccelerationStructure accelerationStructure = g_AccelerationStructures[g_ResourceIndices.AccelerationStructureIndex];
    
    if (sceneConstants.FrameIndex == 1)
    {
        // The accumulation has been reset
        renderTarget[rayID.xy] = float4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    
    // Generate a unique seed for the current pixel and shoot a ray from the camera
    uint seed = GenerateRandomSeed(rayID.x + rayID.y * rayDimensions.x, sceneConstants.FrameIndex);

    float2 pixelPositionCS = (float2)rayID.xy / (float2)rayDimensions.xy * 2.0 - 1.0; // Clip Space [-1, 1]
    float4 pixelPositionVS = mul(sceneConstants.InvProjMatrix, float4(pixelPositionCS.x, -pixelPositionCS.y, 0.0, 1.0)); // To View Space
    
    float3 rayOrigin = sceneConstants.CameraPosition;
    float3 rayDirection = mul(sceneConstants.InvViewMatrix, float4(normalize(pixelPositionVS.xyz / pixelPositionVS.w), 0.0)).xyz; // To World Space
    
    ColorRayPayload payload = TraceColorRay(rayOrigin, rayDirection, seed, 0, accelerationStructure);
    
    // Check for NaN values: Not ideal, we should investigate why some pixels are NaN
    bool colorsNan = any(isnan(payload.Color));
    payload.Color = colorsNan ? float4(0.0, 0.0, 0.0, 1.0) : payload.Color;
    
    // Accumulate color with previous frame
    renderTarget[rayID.xy] = ((sceneConstants.FrameIndex - 1) * prevFrameRenderTarget[rayID.xy] + payload.Color) / sceneConstants.FrameIndex;
}

[shader("closesthit")]
void ClosestHitShader_Color(inout ColorRayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    SceneConstants sceneConstants = g_Buffers[g_ResourceIndices.SceneBufferIndex].Load<SceneConstants>(0);
    RaytracingAccelerationStructure accelerationStructure = g_AccelerationStructures[g_ResourceIndices.AccelerationStructureIndex];
    
    HitInfo hitInfo = GetHitInfo(attr);
    
    MaterialConstants material = GetMeshMaterial(InstanceID(), g_ResourceIndices.MaterialBufferIndex);
    if (material.NormalMapIndex != INVALID_DESCRIPTOR_INDEX)
    {
        hitInfo.WorldNormal = GetNormalFromMap(g_Textures[material.NormalMapIndex], hitInfo, 0);
        hitInfo.WorldBitangent = normalize(cross(hitInfo.WorldNormal, float3(0.0, 1.0, 0.0)));
        hitInfo.WorldTangent = cross(hitInfo.WorldBitangent, hitInfo.WorldNormal);
    }
    
    float4 albedo = material.AlbedoMapIndex != INVALID_DESCRIPTOR_INDEX ? g_Textures[material.AlbedoMapIndex].SampleLevel(g_LinearWrapSampler, hitInfo.TexCoords, 0) : material.AlbedoColor;
    float3 finalColor = float3(0.0, 0.0, 0.0);
    
    finalColor += material.EmissiveColor.rgb;
    
    // Direct lighting
    if (sceneConstants.NumLights > 0)
    {
        // Pick a random light
        uint lightIndex = min(int(RandomFloat(payload.Seed) * sceneConstants.NumLights), sceneConstants.NumLights - 1);
        float lightSampleProbability = 1.0 / float(sceneConstants.NumLights);
        
        Light light = GetLight(lightIndex, g_ResourceIndices.LightsBufferIndex);

        // Check if the surface is in shadow
        float3 shadowRayDirection = normalize(light.Position - hitInfo.WorldPosition);
        float shadowRayMaxDistance = length(light.Position - hitInfo.WorldPosition);
            
        bool isVisible = TraceShadowRay(hitInfo.WorldPosition, shadowRayDirection, shadowRayMaxDistance, accelerationStructure);
            
        // Calculate lighting  
        switch (material.MaterialType)
        {
            case MaterialType::Lambert:
            {
                finalColor += CalculateDirectLighting_Lambert(hitInfo, light, albedo.rgb);
                break;
            }
            case MaterialType::Phong:
            {
                float4 specular = material.SpecularColor;
                float shininess = material.Shininess;
                finalColor += CalculateDirectLighting_Phong(hitInfo, WorldRayOrigin(), light, albedo.rgb, specular.rgb, shininess);
                break;
            }
            case MaterialType::PBR:
            {
                float roughness = material.RoughnessMapIndex != INVALID_DESCRIPTOR_INDEX ? g_Textures[material.RoughnessMapIndex].SampleLevel(g_LinearWrapSampler, hitInfo.TexCoords, 0).r : material.Roughness;
                float metalness = material.MetalnessMapIndex != INVALID_DESCRIPTOR_INDEX ? g_Textures[material.MetalnessMapIndex].SampleLevel(g_LinearWrapSampler, hitInfo.TexCoords, 0).r : material.Metalness;
                finalColor += CalculateDirectLighting_PBR(hitInfo, WorldRayOrigin(), light, albedo.rgb, roughness, metalness);
                break;
            }
        }
            
        finalColor *= isVisible;
        finalColor /= lightSampleProbability;
    }
    
    // Indirect Light
    switch (material.MaterialType)
    {
        case MaterialType::Lambert:
        {
            finalColor += CalculateIndirectLighting_Lambert(hitInfo, payload.Seed, payload.RayDepth, albedo.rgb, accelerationStructure);
            break;
        }
        case MaterialType::Phong:
        {
            float4 specular = material.SpecularColor;
            float shininess = material.Shininess;
            finalColor += CalculateIndirectLighting_Phong(hitInfo, payload.Seed, payload.RayDepth, WorldRayOrigin(), albedo.rgb, specular.rgb, shininess, accelerationStructure);
            break;
        }
        case MaterialType::PBR:
        {
            float roughness = material.RoughnessMapIndex != INVALID_DESCRIPTOR_INDEX ? g_Textures[material.RoughnessMapIndex].SampleLevel(g_LinearWrapSampler, hitInfo.TexCoords, 0).r : material.Roughness;
            float metalness = material.MetalnessMapIndex != INVALID_DESCRIPTOR_INDEX ? g_Textures[material.MetalnessMapIndex].SampleLevel(g_LinearWrapSampler, hitInfo.TexCoords, 0).r : material.Metalness;
            finalColor += CalculateIndirectLighting_PBR(hitInfo, payload.Seed, payload.RayDepth, WorldRayOrigin(), albedo.rgb, roughness, metalness, accelerationStructure);
            break;
        }
    }
    
    payload.Color.rgb += finalColor;
}

[shader("miss")]
void MissShader_Color(inout ColorRayPayload payload)
{
    TextureCube environmentMap = g_CubeMaps[g_ResourceIndices.EnvironmentMapIndex];
    payload.Color += environmentMap.SampleLevel(g_LinearWrapSampler, WorldRayDirection(), 0);
}

[shader("miss")]
void MissShader_Shadow(inout ShadowRayPayload payload)
{
    payload.Visible = true;
}

