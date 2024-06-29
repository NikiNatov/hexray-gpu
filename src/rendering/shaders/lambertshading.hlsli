#ifndef __LAMBERTSHADING_HLSLI__
#define __LAMBERTSHADING_HLSLI__

#include "resources.h"
#include "random.hlsli"

float3 CalculateDirectionalLight_Lambert(Light light, float3 N, float3 albedo)
{
    float3 L = normalize(-light.Direction);
    float NDotL = max(dot(N, L), 0.0);
    
    return (albedo * light.Color) * (light.Intensity * NDotL);
}

float3 CalculatePointLight_Lambert(Light light, float3 N, float3 hitPosition, float3 albedo)
{
    // Calculate attenuation
    float3 lightToSurface = hitPosition - light.Position.xyz;
    float distance = length(lightToSurface);
    float attenuation = 1.0 / max(light.AttenuationFactors[0] + light.AttenuationFactors[1] * distance + light.AttenuationFactors[2] * distance * distance, Epsilon);
    
    // Calculate color
    float3 L = normalize(-lightToSurface);
    float NDotL = max(dot(N, L), 0.0);
    
    return (albedo * light.Color) * light.Intensity * attenuation * NDotL;
}

float3 CalculateSpotLight_Lambert(Light light, float3 N, float3 hitPosition, float3 albedo)
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
    
    return (albedo * light.Color) * (light.Intensity * attenuation * spotIntensity * NDotL);
}

float3 CalculateDirectLighting_Lambert(HitInfo hitInfo, Light light, float3 albedo)
{
    switch (light.LightType)
    {
        case LightType::DirLight:
            return CalculateDirectionalLight_Lambert(light, hitInfo.WorldNormal, albedo);
        case LightType::PointLight:
            return CalculatePointLight_Lambert(light, hitInfo.WorldNormal, hitInfo.WorldPosition, albedo);
        case LightType::SpotLight:
            return CalculateSpotLight_Lambert(light, hitInfo.WorldNormal, hitInfo.WorldPosition, albedo);
    }
    
    return float3(0.0, 0.0, 0.0);
}

float3 CalculateIndirectLighting_Lambert(HitInfo hitInfo, inout uint seed, uint recursionDepth, float3 albedo, RaytracingAccelerationStructure accelerationStruct)
{
    float3 N = hitInfo.WorldNormal;
    
    // For diffuse lighting, pick a random cosine-weighted direction.
    float3 L = GetRandomDirectionCosineWeighted(seed, hitInfo.WorldNormal, hitInfo.WorldTangent, hitInfo.WorldBitangent);
    float NDotL = max(dot(N, L), 0.0);
    
    ColorRayPayload diffusePayload = TraceColorRay(hitInfo.WorldPosition, L, seed, recursionDepth, accelerationStruct);
    
    float3 diffuseBRDF = albedo;
    float cosineSampleProbability = NDotL / PI;
    
    return (diffuseBRDF * diffusePayload.Color.rgb * NDotL) / max(cosineSampleProbability, Epsilon);
}

//float3 CalculateIndirectLighting_Lambert(HitInfo hitInfo, inout uint seed, uint recursionDepth, float3 albedo, RaytracingAccelerationStructure accelerationStruct)
//{
//    float3 N = hitInfo.WorldNormal;
//    
//    // For diffuse lighting, pick a random cosine-weighted direction.
//    float3 L = GetRandomDirectionCosineWeighted(seed, hitInfo.WorldNormal, hitInfo.WorldTangent, hitInfo.WorldBitangent);
//    float NDotL = dot(N, L);
//    float3 brdfColor = albedo * (1.0 / PI) * NDotL;
//    float brdfPdf = 1.0 / (2.0 * PI);
//
//    ColorRayPayload diffusePayload = TraceColorRay(hitInfo.WorldPosition, L, seed, recursionDepth, accelerationStruct);
//    float3 fromGI = diffusePayload.Color.rgb * (brdfColor / brdfPdf);
//
//    float3 eval = albedo * ((1.0 / PI) * abs(NDotL));
//    
//    return fromGI + eval;
//}

#endif // __LAMBERTSHADING_HLSLI__