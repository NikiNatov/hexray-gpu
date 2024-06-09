#ifndef __LAMBERTSHADING_HLSLI__
#define __LAMBERTSHADING_HLSLI__

#include "BindlessResources.hlsli"

float3 CalculateDirectionalLight_Lambert(Light light, float3 diffuseColor, float3 normal)
{
    float lambertianTerm = max(0.0, dot(normal, -light.Direction));
    return lambertianTerm * diffuseColor * light.Color * light.Intensity;
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
    uint triangleIndex = PrimitiveIndex();
    
    MaterialConstants material = GetMeshMaterial(geometryID, g_ResourceIndices.MaterialBufferIndex);
    GeometryConstants geometry = GetMesh(geometryID, g_ResourceIndices.GeometryBufferIndex);
    SceneConstants sceneConstants = g_Buffers[g_ResourceIndices.SceneBufferIndex].Load<SceneConstants>(0);
    
    float3 bary = float3(1.0 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    Triangle tri = GetTriangle(geometry, triangleIndex);
    Vertex ip = GetIntersectionPoint(tri, bary);
    
    float3 surfacePositionWS = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float3 normalWS = normalize(mul((float3x3) ObjectToWorld4x3(), ip.Normal));
    
    float4 diffuseColor = material.AlbedoMapIndex != INVALID_DESCRIPTOR_INDEX ? g_Textures[material.AlbedoMapIndex].SampleLevel(g_LinearWrapSampler, ip.TexCoord, 0) : material.AlbedoColor;
    
    float3 directLightColor = float3(0.0f, 0.0f, 0.0f);
    
    for (uint i = 0; i < sceneConstants.NumLights; i++)
    {
        Light light = GetLight(i, g_ResourceIndices.LightsBufferIndex);
        if (light.LightType == DIRECTIONAL_LIGHT)
        {
            directLightColor += CalculateDirectionalLight_Lambert(light, diffuseColor.xyz, normalWS);
        }
        else if (light.LightType == POINT_LIGHT)
        {
            directLightColor += CalculatePointLight_Lambert(light, diffuseColor.xyz, surfacePositionWS, normalWS);
        }
        else if (light.LightType == SPOT_LIGHT)
        {
            directLightColor += CalculateSpotLight_Lambert(light, diffuseColor.xyz, surfacePositionWS, normalWS);
        }
    }
    
    float3 ambientLightColor = diffuseColor.xyz * float3(0.2f, 0.2f, 0.2f);
    
    payload.Color = float4(directLightColor + ambientLightColor, diffuseColor.a);
}

#endif // __LAMBERTSHADING_HLSLI__