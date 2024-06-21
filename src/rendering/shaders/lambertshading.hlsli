#ifndef __LAMBERTSHADING_HLSLI__
#define __LAMBERTSHADING_HLSLI__

#include "resources.h"

float3 CalculateDirectionalLight_Lambert(Light light, float3 N, float3 albedo)
{
    float3 L = normalize(-light.Direction);
    float NDotL = max(dot(N, L), 0.0);
    
    // Diffuse BRDF
    float3 diffuseBRDF = albedo;
    return diffuseBRDF * light.Color * light.Intensity * NDotL;
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
    
    // Diffuse BRDF
    float3 diffuseBRDF = albedo;
    return diffuseBRDF * light.Color * light.Intensity * attenuation * NDotL;
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
    
    // Diffuse BRDF
    float3 diffuseBRDF = albedo;
    return diffuseBRDF * light.Color * light.Intensity * attenuation * spotIntensity * NDotL;
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

#endif // __LAMBERTSHADING_HLSLI__