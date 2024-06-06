#include "BindlessResources.hlsli"

struct RayPayload
{
    float4 Color;
};

inline float3 GenerateCameraRay(uint2 index, in float3 cameraPosition, in float4x4 projectionToWorld)
{
    float2 xy = index + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

    // Invert Y for DirectX-style coordinates.
    screenPos.y = -screenPos.y;

    // Unproject the pixel coordinate into a world positon.
    float4 world = mul(projectionToWorld, float4(screenPos, 0, 1));
    world.xyz /= world.w;

    return normalize(world.xyz - cameraPosition);
}

[shader("raygeneration")]
void RayGenShader()
{
    RWTexture2D<float4> renderTarget = g_RWTextures[g_ResourceIndices.RenderTargetIndex];
    RaytracingAccelerationStructure accelerationStructure = g_AccelerationStructures[g_ResourceIndices.AccelerationStructureIndex];
    SceneConstants sceneConstants = g_Buffers[g_ResourceIndices.SceneBufferIndex].Load<SceneConstants>(0);
    
    RayDesc ray;
    ray.Origin = sceneConstants.CameraPosition;
    ray.Direction = GenerateCameraRay(DispatchRaysIndex().xy, sceneConstants.CameraPosition, sceneConstants.InvViewProjMatrix);
    ray.TMin = 0.001;
    ray.TMax = 1000.0;
    
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
    
    uint i0 = GetMeshIndex(triangleIndex * 3 + 0, geometry.IndexBufferIndex);
    uint i1 = GetMeshIndex(triangleIndex * 3 + 1, geometry.IndexBufferIndex);
    uint i2 = GetMeshIndex(triangleIndex * 3 + 2, geometry.IndexBufferIndex);
    
    Vertex v0 = GetMeshVertex(i0, geometry.VertexBufferIndex);
    Vertex v1 = GetMeshVertex(i1, geometry.VertexBufferIndex);
    Vertex v2 = GetMeshVertex(i2, geometry.VertexBufferIndex);
    
    float3 bary = float3(1.0 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    float2 texCoord = bary.x * v0.TexCoord + bary.y * v1.TexCoord + bary.z * v2.TexCoord;
    
    // Note: Seems like diffuse.Sample is not supported
    Texture2D diffuse = g_Textures[material.AlbedoMapIndex];
    payload.Color = diffuse.SampleLevel(g_LinearWrapSampler, texCoord, 0) * material.AlbedoColor;
}

[shader("miss")]
void MissShader(inout RayPayload payload)
{
    payload.Color = float4(0.1, 0.2, 0.6, 1);
}