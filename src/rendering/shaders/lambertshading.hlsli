#ifndef __LAMBERTSHADING_HLSLI__
#define __LAMBERTSHADING_HLSLI__

#include "BindlessResources.hlsli"
#include "common.hlsli"

float3 CalculateDirectionalLight_Lambert(Light light, float3 diffuseColor, float3 normal, bool isInShadow)
{
    float shadowFactor = isInShadow ? 0.35f : 1.0;
    float lambertianTerm = max(0.0, dot(normal, -light.Direction));
    return shadowFactor * lambertianTerm * diffuseColor * light.Color * light.Intensity;
}

float3 CalculatePointLight_Lambert(Light light, float3 diffuseColor, float3 surfacePosition, float3 normal)
{
    float3 lightToSurface = surfacePosition - light.Position;
    float distance = length(lightToSurface);
    lightToSurface = normalize(lightToSurface);
    
    float attenuation = 1.0f / max(0.0001, light.AttenuationFactors[0] + light.AttenuationFactors[1] * distance + light.AttenuationFactors[2] * distance * distance);
    float lambertianTerm = max(0.0, dot(normal, -lightToSurface));
    
    return lambertianTerm * diffuseColor * light.Color * light.Intensity * attenuation;
}

float3 CalculateSpotLight_Lambert(Light light, float3 diffuseColor, float3 surfacePosition, float3 normal)
{
    float3 lightToSurface = surfacePosition - light.Position;
    float distance = length(lightToSurface);
    lightToSurface = normalize(lightToSurface);
    
    float minCos = cos(light.ConeAngle);
    float maxCos = (minCos + 1.0) / 2.0;
    float cosAngle = dot(light.Direction.xyz, lightToSurface);
    float spotIntensity = smoothstep(minCos, maxCos, cosAngle);

    float attenuation = 1.0 / max(0.0001, light.AttenuationFactors[0] + light.AttenuationFactors[1] * distance + light.AttenuationFactors[2] * distance * distance);
    float lambertianTerm = max(0.0, dot(normal, -lightToSurface));

    return lambertianTerm * diffuseColor * light.Color * light.Intensity * spotIntensity * attenuation;
}

[shader("closesthit")]
void ClosestHitShader_Lambert(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    uint geometryID = InstanceID();

    MaterialConstants material = GetMeshMaterial(geometryID, g_ResourceIndices.MaterialBufferIndex);
    GeometryConstants geometry = GetMesh(geometryID, g_ResourceIndices.GeometryBufferIndex);
    SceneConstants sceneConstants = g_Buffers[g_ResourceIndices.SceneBufferIndex].Load<SceneConstants>(0);
    
    float3 bary = float3(1.0 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    Triangle tri = GetTriangle(geometry, PrimitiveIndex());
    Vertex ip = GetIntersectionPointOS(tri, bary);
    Vertex ipWS = GetIntersectionPointWS(ip);
    
    SampleGradValues sampleValues = GetSamplingValues(sceneConstants, tri, ipWS.Position, ipWS.Normal, ip.TexCoord);
    
    if (material.NormalMapIndex != INVALID_DESCRIPTOR_INDEX)
    {
        ipWS.Normal = GetNormalFromMap(g_Textures[material.NormalMapIndex], ipWS, sampleValues);
    }

    float4 diffuseColor = material.AlbedoMapIndex != INVALID_DESCRIPTOR_INDEX ? SampleTextureGrad(g_Textures[material.AlbedoMapIndex], g_LinearWrapSampler, sampleValues) : material.AlbedoColor;
    
    float3 directLightColor = float3(0.0f, 0.0f, 0.0f);
    
    for (uint i = 0; i < sceneConstants.NumLights; i++)
    {
        Light light = GetLight(i, g_ResourceIndices.LightsBufferIndex);
        
        if (light.LightType == DIRECTIONAL_LIGHT)
        {
            // Cast a shadow ray
            Ray shadowRay;
            shadowRay.Origin = ipWS.Position;
            shadowRay.Direction = normalize(-light.Direction);
        
            bool isInShadow = TraceShadowRay(shadowRay, payload.RecursionDepth);
            
            directLightColor += CalculateDirectionalLight_Lambert(light, diffuseColor.xyz, ipWS.Normal, isInShadow);
        }
        else if (light.LightType == POINT_LIGHT)
        {
            directLightColor += CalculatePointLight_Lambert(light, diffuseColor.xyz, ipWS.Position, ipWS.Normal);
        }
        else if (light.LightType == SPOT_LIGHT)
        {
            directLightColor += CalculateSpotLight_Lambert(light, diffuseColor.xyz, ipWS.Position, ipWS.Normal);
        }
    }
    
    float3 ambientLightColor = diffuseColor.xyz * float3(0.2f, 0.2f, 0.2f);
    
    payload.Color = float4(directLightColor + ambientLightColor, diffuseColor.a);
}

#endif // __LAMBERTSHADING_HLSLI__