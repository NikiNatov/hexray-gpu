#ifndef __PHONGSHADING_HLSLI__
#define __PHONGSHADING_HLSLI__

#include "BindlessResources.hlsli"
#include "common.hlsli"

float3 CalculateDirectionalLight_Phong(Light light, float4 albedo, float4 specular, float shininess, float3 surfacePosition, float3 viewDir, float3 normal, bool isInShadow)
{
    // Diffuse
    float shadowFactor = isInShadow ? 0.35f : 1.0;
    float lambertianTerm = max(0.0, dot(normal, -light.Direction));
    float3 diffuseLight = shadowFactor * albedo.rgb * lambertianTerm * light.Color * light.Intensity;
    
    // Specular
    float3 specularLight = float3(0.0f, 0.0f, 0.0f);
    if(!isInShadow)
    {
        float3 reflectedLightDir = reflect(light.Direction, normal);
        float specularTerm = pow(max(0.0, dot(-viewDir, reflectedLightDir)), shininess);
        float3 specularLight = specular.rgb * specularTerm * light.Color * light.Intensity;
    }
    
    return diffuseLight + specularLight;
}

float3 CalculatePointLight_Phong(Light light, float4 albedo, float4 specular, float shininess, float3 surfacePosition, float3 viewDir, float3 normal)
{
    float3 lightToSurface = surfacePosition - light.Position;
    float distance = length(lightToSurface);
    lightToSurface = normalize(lightToSurface);
    
    float attenuation = 1.0f / max(0.0001, light.AttenuationFactors[0] + light.AttenuationFactors[1] * distance + light.AttenuationFactors[2] * distance * distance);
    
    // Diffuse
    float lambertianTerm = max(0.0, dot(normal, -lightToSurface));
    float3 diffuseLight = albedo.rgb * lambertianTerm * light.Color * light.Intensity * attenuation;
    
    // Specular
    float3 reflectedLightDir = reflect(lightToSurface, normal);
    float specularTerm = pow(max(0.0, dot(-viewDir, reflectedLightDir)), shininess);
    float3 specularLight = specular.rgb * specularTerm * light.Color * light.Intensity * attenuation;
    
    return diffuseLight + specularLight;
}

float3 CalculateSpotLight_Phong(Light light, float4 albedo, float4 specular, float shininess, float3 surfacePosition, float3 viewDir, float3 normal)
{
    float3 lightToSurface = surfacePosition - light.Position;
    float distance = length(lightToSurface);
    lightToSurface = normalize(lightToSurface);
    
    float minCos = cos(light.ConeAngle);
    float maxCos = (minCos + 1.0) / 2.0;
    float cosAngle = dot(light.Direction.xyz, normalize(lightToSurface));
    float spotIntensity = smoothstep(minCos, maxCos, cosAngle);

    float attenuation = 1.0 / max(0.0001, light.AttenuationFactors[0] + light.AttenuationFactors[1] * distance + light.AttenuationFactors[2] * distance * distance);
    
    // Diffuse
    float lambertianTerm = max(0.0, dot(normal, -lightToSurface));
    float3 diffuseLight = albedo.rgb * lambertianTerm * light.Color * light.Intensity * spotIntensity * attenuation;
    
    // Specular
    float3 reflectedLightDir = reflect(lightToSurface, normal);
    float specularTerm = pow(max(0.0, dot(-viewDir, reflectedLightDir)), shininess);
    float3 specularLight = specular.rgb * specularTerm * light.Color * light.Intensity * spotIntensity * attenuation;
    
    return diffuseLight + specularLight;
}

[shader("closesthit")]
void ClosestHitShader_Phong(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
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
    float3 tangentWS = normalize(mul((float3x3) ObjectToWorld4x3(), ip.Tangent));
    float3 bitangentWS = normalize(mul((float3x3) ObjectToWorld4x3(), ip.Bitangent));
    
    if (material.NormalMapIndex != INVALID_DESCRIPTOR_INDEX)
    {
        normalWS = GetNormalFromMap(g_Textures[material.NormalMapIndex], ip.TexCoord, tangentWS, bitangentWS, normalWS);
    }
    
    float4 albedoColor = material.AlbedoMapIndex != INVALID_DESCRIPTOR_INDEX ? g_Textures[material.AlbedoMapIndex].SampleLevel(g_LinearWrapSampler, ip.TexCoord, 0) : material.AlbedoColor;
    float4 specularColor = material.SpecularColor;
    float shininess = material.Shininess;
    
    float3 viewDir = normalize(surfacePositionWS - sceneConstants.CameraPosition);
    
    float3 directLightColor = float3(0.0f, 0.0f, 0.0f);
    
    for (uint i = 0; i < sceneConstants.NumLights; i++)
    {
        Light light = GetLight(i, g_ResourceIndices.LightsBufferIndex);
        if (light.LightType == DIRECTIONAL_LIGHT)
        {
            // Cast a shadow ray
            Ray shadowRay;
            shadowRay.Origin = surfacePositionWS;
            shadowRay.Direction = normalize(-light.Direction);
        
            bool isInShadow = TraceShadowRay(shadowRay, payload.RecursionDepth);
            
            directLightColor += CalculateDirectionalLight_Phong(light, albedoColor, specularColor, shininess, surfacePositionWS, viewDir, normalWS, isInShadow);
        }
        else if (light.LightType == POINT_LIGHT)
        {
            directLightColor += CalculatePointLight_Phong(light, albedoColor, specularColor, shininess, surfacePositionWS, viewDir, normalWS);
        }
        else if (light.LightType == SPOT_LIGHT)
        {
            directLightColor += CalculateSpotLight_Phong(light, albedoColor, specularColor, shininess, surfacePositionWS, viewDir, normalWS);
        }
    }
    
    float3 ambientLightColor = albedoColor.xyz * float3(0.2f, 0.2f, 0.2f);
    
    payload.Color = float4(directLightColor + ambientLightColor, albedoColor.a);
}

#endif // __PHONGSHADING_HLSLI__