#ifndef __PRBSHADING_HLSLI__
#define __PRBSHADING_HLSLI__

#include "BindlessResources.hlsli"
#include "common.hlsli"

//GGX/Trowbridge-Reitz normal distribution function
float NormalDistributionFunction(float alpha, float3 N, float3 H)
{
    float NDotH = max(dot(N, H), 0.0);

    float numerator = alpha * alpha;
    float denominator = max(PI * pow(pow(NDotH, 2.0) * (pow(alpha, 2.0) - 1.0) + 1.0, 2.0), Epsilon);

    return numerator / denominator;
}

//Schlick-GGX term
float SchlickG1(float k, float3 N, float3 X)
{
    float NDotX = max(dot(N, X), 0.0);

    float numerator = NDotX;
    float denominator = max(NDotX * (1.0 - k) + k, Epsilon);

    return numerator / denominator;
}

//Schlick-GGX geometry shadowing function
float GeometryShadowingFunction(float alpha, float3 N, float3 L, float3 V)
{
    float k = pow(alpha + 1.0, 2.0) / 8.0;
    return SchlickG1(k, N, V) * SchlickG1(k, N, L);
}

//Fresnel-Schlick equation
float3 FresnelSchlickFunction(float3 F0, float3 V, float3 H, float roughness)
{
    float VDotH = max(dot(V, H), 0.0);
    return F0 + (float3(1.0, 1.0, 1.0) - F0) * pow(1.0 - VDotH, 5.0);
}

//Cook-Torrance equation
float3 CookTorranceFunction(float roughness, float3 F0, float3 N, float3 V, float3 L, float3 H)
{
    float3 numerator = NormalDistributionFunction(roughness, N, H) * GeometryShadowingFunction(roughness, N, L, V) * FresnelSchlickFunction(F0, V, H, roughness);
    float denominator = max(4.0 * max(dot(V, N), 0.0) * max(dot(L, N), 0.0), Epsilon);

    return numerator / denominator;
}

float3 CalculateDirectionalLight_PBR(Light light, float3 F0, float3 V, float3 N, float3 albedoColor, float metalness, float roughness, bool isInShadow)
{
    float shadowFactor = isInShadow ? 0.35f : 1.0;
    
    float3 L = normalize(-light.Direction.xyz);
    float3 H = normalize(V + L);

    float3 Ks = FresnelSchlickFunction(F0, V, H, roughness);
    float3 Kd = (float3(1.0, 1.0, 1.0) - Ks) * (1.0 - metalness);

    float3 specularColor = CookTorranceFunction(roughness, F0, N, V, L, H);
    float3 brdf = Kd * albedoColor / PI + specularColor;

    return shadowFactor * brdf * light.Color.rgb * light.Intensity * max(dot(L, N), 0.0);
}

float3 CalculatePointLight_PBR(Light light, float3 F0, float3 V, float3 N, float3 surfacePosition, float3 albedoColor, float metalness, float roughness)
{
    float3 L = light.Position.xyz - surfacePosition;
    float distance = length(L);
    L = normalize(L);
    float3 H = normalize(V + L);

    float3 Ks = FresnelSchlickFunction(F0, V, H, roughness);
    float3 Kd = (float3(1.0, 1.0, 1.0) - Ks) * (1.0 - metalness);

    float attenuation = 1.0f / max(light.AttenuationFactors[0] + light.AttenuationFactors[1] * distance + light.AttenuationFactors[2] * distance * distance, Epsilon);
    float3 specularColor = CookTorranceFunction(roughness, F0, N, V, L, H);
    float3 brdf = Kd * albedoColor / PI + specularColor;

    return brdf * light.Color.rgb * light.Intensity * attenuation * max(dot(L, N), 0.0);
}

float3 CalculateSpotLight_PBR(Light light, float3 F0, float3 V, float3 N, float3 surfacePosition, float3 albedoColor, float metalness, float roughness)
{
    float3 L = light.Position.xyz - surfacePosition;
    float distance = length(L);
    L = normalize(L);
    float3 H = normalize(V + L);

    float3 Ks = FresnelSchlickFunction(F0, V, H, roughness);
    float3 Kd = (float3(1.0, 1.0, 1.0) - Ks) * (1.0 - metalness);

    // Calculate spot intensity
    float minCos = cos(light.ConeAngle);
    float maxCos = (minCos + 1.0f) / 2.0f;
    float cosAngle = dot(light.Direction.xyz, -L);
    float spotIntensity = smoothstep(minCos, maxCos, cosAngle);

    float attenuation = 1.0f / max(light.AttenuationFactors[0] + light.AttenuationFactors[1] * distance + light.AttenuationFactors[2] * distance * distance, Epsilon);
    float3 specularColor = CookTorranceFunction(roughness, F0, N, V, L, H);
    float3 brdf = Kd * albedoColor / PI + specularColor;

    return brdf * light.Color.rgb * light.Intensity * spotIntensity * attenuation * max(dot(L, N), 0.0);
}

[shader("closesthit")]
void ClosestHitShader_PBR(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
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

    float4 albedoColor = material.AlbedoMapIndex != INVALID_DESCRIPTOR_INDEX ? SampleTextureGrad(g_Textures[material.AlbedoMapIndex], g_LinearWrapSampler, sampleValues) : material.AlbedoColor;
    float metalness = material.MetalnessMapIndex != INVALID_DESCRIPTOR_INDEX ? SampleTextureGrad(g_Textures[material.MetalnessMapIndex], g_LinearWrapSampler, sampleValues) : material.Metalness;
    float roughness = material.RoughnessMapIndex != INVALID_DESCRIPTOR_INDEX ? SampleTextureGrad(g_Textures[material.RoughnessMapIndex], g_LinearWrapSampler, sampleValues) : material.Roughness;

    float3 N = ipWS.Normal;
    float3 V = normalize(sceneConstants.CameraPosition - ipWS.Position);
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedoColor.rgb, metalness);

    // Lights
    float3 directLightColor = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < sceneConstants.NumLights; i++)
    {
        Light light = GetLight(i, g_ResourceIndices.LightsBufferIndex);
        
        if (light.LightType == DIRECTIONAL_LIGHT)
        {
            Ray shadowRay;
            shadowRay.Origin = ipWS.Position;
            shadowRay.Direction = normalize(-light.Direction);
        
            bool isInShadow = TraceShadowRay(shadowRay, payload.RecursionDepth);
                
            directLightColor += CalculateDirectionalLight_PBR(light, F0, V, N, albedoColor.rgb, metalness, roughness, isInShadow);
        }
        else if (light.LightType == POINT_LIGHT)
        {
            directLightColor += CalculatePointLight_PBR(light, F0, V, N, ipWS.Position, albedoColor.rgb, metalness, roughness);
        }
        else if (light.LightType == SPOT_LIGHT)
        {
            directLightColor += CalculateSpotLight_PBR(light, F0, V, N, ipWS.Position, albedoColor.rgb, metalness, roughness);
        }
    }
    
    float3 ambientLightColor = albedoColor.xyz * float3(0.2f, 0.2f, 0.2f);
    payload.Color = float4(directLightColor + ambientLightColor, albedoColor.a);
}

#endif // __PRBSHADING_HLSLI__