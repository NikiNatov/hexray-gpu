#ifndef __PHONGSHADING_HLSLI__
#define __PHONGSHADING_HLSLI__

#include "resources.h"
#include "random.hlsli"

float3 CalculateDirectionalLight_Phong(Light light, float3 V, float3 N, float3 albedo, float3 specular, float shininess)
{
    float3 L = normalize(-light.Direction);
    float NDotL = max(dot(N, L), 0.0);
    
    // Diffuse BRDF
    float3 diffuseBRDF = albedo;
    
    // Specular BRDF
    float specularTerm = pow(max(dot(V, reflect(-L, N)), 0.0), shininess);
    float3 specularBRDF = specularTerm * specular;
    
    return (diffuseBRDF + specularBRDF) * light.Color * light.Intensity * NDotL;
}

float3 CalculatePointLight_Phong(Light light, float3 V, float3 N, float3 hitPosition, float3 albedo, float3 specular, float shininess)
{
    // Calculate attenuation
    float3 lightToSurface = hitPosition - light.Position.xyz;
    float distance = length(lightToSurface);
    float attenuation = 1.0 / max(light.AttenuationFactors[0] + light.AttenuationFactors[1] * distance + light.AttenuationFactors[2] * distance * distance, Epsilon);
    
    // Calculate color
    float3 L = normalize(-lightToSurface);
    float NDotL = max(dot(N, L), 0.0);
    
    // Diffuse BRDF
    float3 diffuseBRDF = albedo;
    
    // Specular BRDF
    float specularTerm = pow(max(dot(V, reflect(-L, N)), 0.0), shininess);
    float3 specularBRDF = specularTerm * specular.rgb;
    
    return (diffuseBRDF + specularBRDF) * light.Color * light.Intensity * attenuation * NDotL;
}

float3 CalculateSpotLight_Phong(Light light, float3 V, float3 N, float3 hitPosition, float3 albedo, float3 specular, float shininess)
{
    // Calculate attenuation
    float3 lightToSurface = hitPosition - light.Position.xyz;
    float distance = length(lightToSurface);
    float attenuation = 1.0 / max(light.AttenuationFactors[0] + light.AttenuationFactors[1] * distance + light.AttenuationFactors[2] * distance * distance, Epsilon);
    
    // Calculate spot intensity
    float minCos = cos(light.ConeAngle);
    float maxCos = (minCos + 1.0) / 2.0;
    float cosAngle = dot(light.Direction.xyz, normalize(lightToSurface));
    float spotIntensity = smoothstep(minCos, maxCos, cosAngle);

    // Calculate color
    float3 L = normalize(-lightToSurface);
    float NDotL = max(dot(N, L), 0.0);
    
    // Diffuse BRDF
    float3 diffuseBRDF = albedo;
    
    // Specular BRDF
    float specularTerm = pow(max(dot(V, reflect(-L, N)), Epsilon), shininess);
    float3 specularBRDF = specularTerm * specular.rgb;
    
    return (diffuseBRDF + specularBRDF) * light.Color * light.Intensity * attenuation * spotIntensity * NDotL;
}

float3 CalculateDirectLighting_Phong(HitInfo hitInfo, float3 cameraPosition, Light light, float3 albedo, float3 specular, float shininess)
{
    float3 viewDir = normalize(hitInfo.WorldPosition - cameraPosition);
    
    switch (light.LightType)
    {
        case LightType::DirLight:
            return CalculateDirectionalLight_Phong(light, -viewDir, hitInfo.WorldNormal, albedo, specular, shininess);
        case LightType::PointLight:
            return CalculatePointLight_Phong(light, -viewDir, hitInfo.WorldNormal, hitInfo.WorldPosition, albedo, specular, shininess);
        case LightType::SpotLight:
            return CalculateSpotLight_Phong(light, -viewDir, hitInfo.WorldNormal, hitInfo.WorldPosition, albedo, specular, shininess);
    }
    
    return float3(0.0, 0.0, 0.0);
}

float3 CalculateIndirectLighting_Phong(HitInfo hitInfo, inout uint seed, uint recursionDepth, float3 cameraPosition, float3 albedo, float3 specular, float shininess, RaytracingAccelerationStructure accelerationStruct)
{
    float3 N = hitInfo.WorldNormal;
    
    float3 diffuseLight = float3(0.0, 0.0, 0.0);
    {
        // For diffuse lighting, pick a random cosine-weighted direction.
        float3 L = GetRandomDirectionCosineWeighted(seed, hitInfo.WorldNormal, hitInfo.WorldTangent, hitInfo.WorldBitangent);
        float NDotL = max(dot(N, L), 0.0);
        
        ColorRayPayload diffusePayload = TraceColorRay(hitInfo.WorldPosition, L, seed, recursionDepth, accelerationStruct);
        
        float3 diffuseBRDF = albedo / PI;
        float cosineSampleProbability = NDotL / PI;
        
        diffuseLight = (diffuseBRDF * diffusePayload.Color.rgb * NDotL) / max(cosineSampleProbability, Epsilon);
    }
    
    float3 specularLight = float3(0.0, 0.0, 0.0);
    {
        // For specular lighting, pick a random direction using the Blinn-Phong distribution function. This is effectively the microfacet half-vector
        float3 H = GetRandomDirectionBlinnPhong(seed, shininess, hitInfo.WorldNormal, hitInfo.WorldTangent, hitInfo.WorldBitangent);
        float3 V = normalize(cameraPosition - hitInfo.WorldPosition);
        
        float VDotH = max(dot(V, H), 0.0);
        
        float3 L = normalize(2.0 * VDotH * H - V);
    
        ColorRayPayload specularPayload = TraceColorRay(hitInfo.WorldPosition, L, seed, recursionDepth, accelerationStruct);
        
        float NDotL = max(dot(N, L), 0.0);
        float NDotH = max(dot(N, H), 0.0);
    
        float specularTerm = pow(NDotH, shininess);
        float3 specularBRDF = specularTerm * specular;
    
        float blinnPhongSampleProbability = (shininess + 2.0) * specularTerm / (2.0 * PI);
    
        specularLight = (specularBRDF * specularPayload.Color.rgb * NDotL) / max(blinnPhongSampleProbability, Epsilon);
    }
    
    return diffuseLight + specularLight;
}

#endif // __PHONGSHADING_HLSLI__