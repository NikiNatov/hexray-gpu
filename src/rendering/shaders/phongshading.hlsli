#ifndef __PHONGSHADING_HLSLI__
#define __PHONGSHADING_HLSLI__

#include "resources.h"

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
    float specularTerm = pow(max(dot(V, reflect(-L, N)), 0.0), shininess);
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

#endif // __PHONGSHADING_HLSLI__