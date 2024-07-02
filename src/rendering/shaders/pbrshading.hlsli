#ifndef __PRBSHADING_HLSLI__
#define __PRBSHADING_HLSLI__

#include "resources.h"
#include "common.hlsli"
#include "random.hlsli"

// GGX/Trowbridge-Reitz normal distribution function
float NormalDistributionFunction(float alpha, float NDotH)
{
    float a2 = alpha * alpha;
    return a2 / max(PI * Pow2(NDotH * NDotH * (a2 - 1.0) + 1.0), Epsilon);
}

// Schlick-GGX geometry shadowing function
float GeometryShadowingFunction(float alpha, float NDotV, float NDotL)
{
    float k = Pow2(alpha + 1.0) / 8.0;
    
    float schlickG1 = NDotV / max(NDotV * (1.0 - k) + k, Epsilon);
    float schlickG2 = NDotL / max(NDotL * (1.0 - k) + k, Epsilon);
    
    return schlickG1 * schlickG2;
}


//Fresnel-Schlick equation
float3 FresnelSchlickFunction(float3 albedo, float metalness, float VDotH)
{
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metalness);
    return F0 + (float3(1.0, 1.0, 1.0) - F0) * Pow5(1.0 - VDotH);
}

float3 CalculateCommonLight_PBR(Light light, float3 L, float3 V, float3 N, float3 albedo, float roughness, float metalness, float attenuation)
{
    float3 H = normalize(V + L);
    
    float NDotL = max(dot(N, L), 0.0);
    float NDotV = max(dot(N, V), 0.0);
    float NDotH = max(dot(N, H), 0.0);
    float VDotH = max(dot(V, H), 0.0);
    
    // Cook-Torrance specular BRDF
    float D = NormalDistributionFunction(roughness, NDotH);
    float G = GeometryShadowingFunction(roughness, NDotV, NDotL);
    float3 F = FresnelSchlickFunction(albedo, metalness, VDotH);
    float3 specularBRDF = (D * G * F) / max(4.0 * NDotV * NDotL, Epsilon);

    // Diffuse BRDF
    float3 Kd = (float3(1.0, 1.0, 1.0) - F) * (1.0 - metalness);
    float3 diffuseBRDF = Kd * albedo.rgb / PI;

    return (diffuseBRDF + specularBRDF) * light.Color.rgb * (light.Intensity * attenuation * NDotL);
}

float3 CalculateDirectionalLight_PBR(Light light, float3 V, float3 N, float3 albedo, float roughness, float metalness)
{
    float3 L = normalize(-light.Direction);
    return CalculateCommonLight_PBR(light, L, V, N, albedo, roughness, metalness, 1.0f);
}

float3 CalculatePointLight_PBR(Light light, float3 V, float3 N, float3 hitPosition, float3 albedo, float roughness, float metalness)
{
    // Calculate attenuation
    float3 lightToSurface = hitPosition - light.Position.xyz;
    float distance = length(lightToSurface);
    float attenuation = 1.0 / max(light.AttenuationFactors[0] + light.AttenuationFactors[1] * distance + light.AttenuationFactors[2] * distance * distance, Epsilon);
   
    float3 L = -lightToSurface / distance;
    return CalculateCommonLight_PBR(light, L, V, N, albedo, roughness, metalness, attenuation);
}

float3 CalculateSpotLight_PBR(Light light, float3 V, float3 N, float3 hitPosition, float3 albedo, float roughness, float metalness)
{
    // Calculate attenuation
    float3 lightToSurface = hitPosition - light.Position.xyz;
    float distance = length(lightToSurface);
    float attenuation = 1.0 / max(light.AttenuationFactors[0] + light.AttenuationFactors[1] * distance + light.AttenuationFactors[2] * distance * distance, Epsilon);
    
    float3 lightToSurfaceNormalized = lightToSurface / distance;

    // Calculate spot intensity
    float cosAngle = dot(light.Direction.xyz, lightToSurfaceNormalized);
    float spotIntensity = smoothstep(light.ConeAngleMax, light.ConeAngleMin, cosAngle);
    
    // Calculate color
    float3 L = -lightToSurfaceNormalized;
    return CalculateCommonLight_PBR(light, L, V, N, albedo, roughness, metalness, spotIntensity * attenuation);
}

float3 CalculateDirectLighting_PBR(HitInfo hitInfo, float3 cameraPosition, Light light, float3 albedo, float roughness, float metalness)
{
    roughness = clamp(roughness, 0.001, 1.0);
    metalness = clamp(metalness, 0.001, 1.0);
    
    float3 viewDir = normalize(hitInfo.WorldPosition - cameraPosition);
    
    switch (light.LightType)
    {
        case LightType::DirLight:
            return CalculateDirectionalLight_PBR(light, -viewDir, hitInfo.WorldNormal, albedo, roughness, metalness);
        case LightType::PointLight:
            return CalculatePointLight_PBR(light, -viewDir, hitInfo.WorldNormal, hitInfo.WorldPosition, albedo, roughness, metalness);
        case LightType::SpotLight:
            return CalculateSpotLight_PBR(light, -viewDir, hitInfo.WorldNormal, hitInfo.WorldPosition, albedo, roughness, metalness);
    }
    
    return float3(0.0, 0.0, 0.0);
}

float3 CalculateIndirectLighting_PBR(HitInfo hitInfo, inout uint seed, uint recursionDepth, float3 cameraPosition, float3 albedo, float roughness, float metalness, RaytracingAccelerationStructure accelerationStruct)
{
    roughness = clamp(roughness, 0.001, 1.0);
    metalness = clamp(metalness, 0.001, 1.0);
    
    float3 N = hitInfo.WorldNormal;
    
    float3 diffuseLight = float3(0.0, 0.0, 0.0);
    float3 Kd = float3(0.0, 0.0, 0.0);
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
        // For specular lighting, pick a random direction using the GGX NDF. This is effectively the microfacet half-vector
        float3 H = GetRandomDirectionGGX(seed, roughness, hitInfo.WorldNormal, hitInfo.WorldTangent, hitInfo.WorldBitangent);
        float3 V = normalize(cameraPosition - hitInfo.WorldPosition);
        
        float VDotH = max(dot(V, H), 0.0);
        
        float3 L = normalize(2.0 * VDotH * H - V);
    
        ColorRayPayload specularPayload = TraceColorRay(hitInfo.WorldPosition, L, seed, recursionDepth, accelerationStruct);
        
        float NDotL = max(dot(N, L), 0.0);
        float NDotH = max(dot(N, H), 0.0);
        float NDotV = max(dot(N, V), 0.0);
    
        // Cook-Torrance specular BRDF
        float D = NormalDistributionFunction(roughness, NDotH);
        float G = GeometryShadowingFunction(roughness, NDotV, NDotL);
        float3 F = FresnelSchlickFunction(albedo, metalness, VDotH);
        float3 specularBRDF = (D * G * F) / max(4.0 * NDotV * NDotL, Epsilon);
        
        Kd = (float3(1.0, 1.0, 1.0) - F) * (1.0 - metalness);
    
        float ggxSampleProbability = (D * NDotH) / max(4.0 * VDotH, Epsilon);
    
        specularLight = (specularBRDF * specularPayload.Color.rgb * NDotL) / max(ggxSampleProbability, Epsilon);
    }
    
    return (Kd * diffuseLight + specularLight);
}

#endif // __PRBSHADING_HLSLI__