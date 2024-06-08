#include "BindlessResources.hlsli"
#include "Common.hlsli"

float3 CalculateDirectionalLight(Light light, float3 diffuseColor, float3 normal)
{
    float lambertianTerm = max(0.0, dot(normal, normalize(-light.Direction)));
    return lambertianTerm * diffuseColor * light.Color * light.Intensity;
}

float3 CalculatePointLight(Light light, float3 diffuseColor, float3 surfacePosition, float3 normal)
{
    float3 lightToSurface = surfacePosition - light.Position;
    float distance = length(lightToSurface);
    
    float attenuation = 1.0f / max(0.0001, light.AttenuationFactors[0] + light.AttenuationFactors[1] * distance + light.AttenuationFactors[2] * distance * distance);
    float lambertianTerm = max(0.0, dot(normal, normalize(-lightToSurface)));
    
    return lambertianTerm * diffuseColor * light.Color * light.Intensity * attenuation;
}

float3 CalculateSpotLight(Light light, float3 diffuseColor, float3 surfacePosition, float3 normal)
{
    float3 lightToSurface = surfacePosition - light.Position;
    float distance = length(lightToSurface);
    
    float minCos = cos(light.ConeAngle);
    float maxCos = (minCos + 1.0f) / 2.0f;
    float cosAngle = dot(light.Direction.xyz, normalize(lightToSurface));
    float spotIntensity = smoothstep(minCos, maxCos, cosAngle);

    float attenuation = 1.0f / max(0.0001, light.AttenuationFactors[0] + light.AttenuationFactors[1] * distance + light.AttenuationFactors[2] * distance * distance);
    float lambertianTerm = max(0.0, dot(normal, normalize(-lightToSurface)));

    return lambertianTerm * diffuseColor * light.Color * light.Intensity * spotIntensity * attenuation;
}

struct RayPayload
{
    float4 Color;
};

[shader("raygeneration")]
void RayGenShader()
{
    RWTexture2D<float4> renderTarget = g_RWTextures[g_ResourceIndices.RenderTargetIndex];
    RaytracingAccelerationStructure accelerationStructure = g_AccelerationStructures[g_ResourceIndices.AccelerationStructureIndex];
    SceneConstants sceneConstants = g_Buffers[g_ResourceIndices.SceneBufferIndex].Load<SceneConstants>(0);
    
    RayDesc ray = GenerateCameraRay(sceneConstants.CameraPosition, sceneConstants.InvProjMatrix, sceneConstants.InvViewMatrix);
    RayPayload payload = { float4(0, 0, 0, 0) };
    
    TraceRay(accelerationStructure, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);

    renderTarget[DispatchRaysIndex().xy] = payload.Color;
}

[shader("closesthit")]
void ClosestHitShader(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    uint geometryID = InstanceID();
    uint triangleIndex = PrimitiveIndex();
    
    MaterialConstants material = GetMeshMaterial(geometryID, g_ResourceIndices.MaterialBufferIndex);
    GeometryConstants geometry = GetMesh(geometryID, g_ResourceIndices.GeometryBufferIndex);
    SceneConstants sceneConstants = g_Buffers[g_ResourceIndices.SceneBufferIndex].Load<SceneConstants>(0);
    
    uint i0 = GetMeshIndex(triangleIndex * 3 + 0, geometry.IndexBufferIndex);
    uint i1 = GetMeshIndex(triangleIndex * 3 + 1, geometry.IndexBufferIndex);
    uint i2 = GetMeshIndex(triangleIndex * 3 + 2, geometry.IndexBufferIndex);
    
    Vertex v0 = GetMeshVertex(i0, geometry.VertexBufferIndex);
    Vertex v1 = GetMeshVertex(i1, geometry.VertexBufferIndex);
    Vertex v2 = GetMeshVertex(i2, geometry.VertexBufferIndex);
    
    float3 bary = float3(1.0 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    float2 texCoord = bary.x * v0.TexCoord + bary.y * v1.TexCoord + bary.z * v2.TexCoord;
    float3 normal = bary.x * v0.Normal + bary.y * v1.Normal + bary.z * v2.Normal;
    
    float3 surfacePositionWS = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float3 normalWS = normalize(mul((float3x3)ObjectToWorld4x3(), normal));
    
    float4 diffuseColor = g_Textures[material.AlbedoMapIndex].SampleLevel(g_LinearWrapSampler, texCoord, 0);
    
    float3 directLightColor = float3(0.0f, 0.0f, 0.0f);
    
    for (uint i = 0; i < sceneConstants.NumLights; i++)
    {
        Light light = GetLight(i, g_ResourceIndices.LightsBufferIndex);
        if (light.LightType == 0)
        {
            // DirLight
            directLightColor += CalculateDirectionalLight(light, diffuseColor.xyz, normalWS);
        }
        else if (light.LightType == 1)
        {
            // PointLight
            directLightColor += CalculatePointLight(light, diffuseColor.xyz, surfacePositionWS, normalWS);
        }
        else if (light.LightType == 2)
        {
            // SpotLight
            directLightColor += CalculateSpotLight(light, diffuseColor.xyz, surfacePositionWS, normalWS);
        }
    }
    
    float3 ambientLightColor = diffuseColor.xyz * float3(0.2f, 0.2f, 0.2f);
    
    payload.Color = float4(directLightColor + ambientLightColor, diffuseColor.a);
    
}

[shader("miss")]
void MissShader(inout RayPayload payload)
{
    TextureCube environmentMap = g_CubeMaps[g_ResourceIndices.EnvironmentMapIndex];
    payload.Color = environmentMap.SampleLevel(g_LinearWrapSampler, WorldRayDirection(), 0);
}