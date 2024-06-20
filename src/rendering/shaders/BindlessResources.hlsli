#ifndef __BINDLESS_RESOURCES_HLSLI__
#define __BINDLESS_RESOURCES_HLSLI__

#define RESOURCE_INDICES_SPACE space0
#define ROTEXTURE2D_SPACE space0
#define ROTEXTURECUBE_SPACE space1
#define SAMPLERSTATE_SPACE space0
#define ROBUFFERS_SPACE space2
#define ACCELERATION_STRUCTURES_SPACE space3
#define RWTEXTURE2D_SPACE space0
#define RWTEXTURECUBE_SPACE space1
#define RWBUFFERS_SPACE space2

#define INVALID_DESCRIPTOR_INDEX 0xffffffff

// -----------------------------------------------------------------------
struct Vertex
{
    float3 Position;
    float2 TexCoord;
    float3 Normal;
    float3 Tangent;
    float3 Bitangent;
};

static const uint c_VertexStructSize = 56;
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
struct Triangle
{
    Vertex V0;
    Vertex V1;
    Vertex V2;
};
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
struct SceneConstants
{
    matrix ViewMatrix;
    matrix ProjectionMatrix;
    matrix InvProjMatrix;
    matrix InvViewMatrix;
    float3 CameraPosition;
    uint NumLights;
};

static const uint c_SceneConstantsStructSize = 272;
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
struct MaterialConstants
{
    float4 AlbedoColor;
    float4 SpecularColor;   // Phong
    float Shininess;        // Phong
    float Roughness;        // PBR
    float Metalness;        // PBR
    uint AlbedoMapIndex;
    uint NormalMapIndex;
    uint RoughnessMapIndex; // PBR
    uint MetalnessMapIndex; // PBR
};

static const uint c_MaterialConstantsStructSize = 60;

// -----------------------------------------------------------------------
struct GeometryConstants
{
    uint MaterialIndex;
    uint VertexBufferIndex;
    uint IndexBufferIndex;
};

static const uint c_GeometryConstantsStructSize = 12;
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
struct Light
{
    float3 Position;
    float3 Direction;
    float3 Color;
    float Intensity;
    float ConeAngle;
    float3 AttenuationFactors;
    uint LightType;
};

static const uint c_LightStructSize = 60;
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
struct ResourceIndices
{
    uint EnvironmentMapIndex;
    uint RenderTargetIndex;
    uint SceneBufferIndex;
    uint LightsBufferIndex;
    uint MaterialBufferIndex;
    uint GeometryBufferIndex;
    uint AccelerationStructureIndex;
};
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
ConstantBuffer<ResourceIndices> g_ResourceIndices          : register(b0, RESOURCE_INDICES_SPACE);
RaytracingAccelerationStructure g_AccelerationStructures[] : register(t0, ACCELERATION_STRUCTURES_SPACE);
ByteAddressBuffer               g_Buffers[]                : register(t0, ROBUFFERS_SPACE);
Texture2D<float4>               g_Textures[]               : register(t0, ROTEXTURE2D_SPACE);
TextureCube                     g_CubeMaps[]               : register(t0, ROTEXTURECUBE_SPACE);
RWTexture2D<float4>             g_RWTextures[]             : register(u0, RWTEXTURE2D_SPACE);
SamplerState                    g_PointClampSampler        : register(s0, SAMPLERSTATE_SPACE);
SamplerState                    g_PointWrapSampler         : register(s1, SAMPLERSTATE_SPACE);
SamplerState                    g_LinearClampSampler       : register(s2, SAMPLERSTATE_SPACE);
SamplerState                    g_LinearWrapSampler        : register(s3, SAMPLERSTATE_SPACE);
SamplerState                    g_AnisoWrapSampler         : register(s4, SAMPLERSTATE_SPACE);
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// Helper functions for loading elements from raw buffers
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
uint GetMeshIndex(uint i, uint indexBufferIndex)
{
    return g_Buffers[indexBufferIndex].Load(i * 4);
}

// -----------------------------------------------------------------------
Vertex GetMeshVertex(uint i, uint vertexBufferIndex)
{
    return g_Buffers[vertexBufferIndex].Load<Vertex>(i * c_VertexStructSize);
}

// -----------------------------------------------------------------------
MaterialConstants GetMeshMaterial(uint i, uint materialBufferIndex)
{
    return g_Buffers[materialBufferIndex].Load<MaterialConstants>(i * c_MaterialConstantsStructSize);
}

// -----------------------------------------------------------------------
GeometryConstants GetMesh(uint i, uint geometryBufferIndex)
{
    return g_Buffers[geometryBufferIndex].Load<GeometryConstants>(i * c_GeometryConstantsStructSize);
}

// -----------------------------------------------------------------------
Light GetLight(uint i, uint lightsBufferIndex)
{
    return g_Buffers[lightsBufferIndex].Load<Light>(i * c_LightStructSize);
}

// -----------------------------------------------------------------------
Triangle GetTriangle(GeometryConstants geometry, uint triangleIndex)
{
    uint i0 = GetMeshIndex(triangleIndex * 3 + 0, geometry.IndexBufferIndex);
    uint i1 = GetMeshIndex(triangleIndex * 3 + 1, geometry.IndexBufferIndex);
    uint i2 = GetMeshIndex(triangleIndex * 3 + 2, geometry.IndexBufferIndex);
    
    Triangle tri;
    tri.V0 = GetMeshVertex(i0, geometry.VertexBufferIndex);
    tri.V1 = GetMeshVertex(i1, geometry.VertexBufferIndex);
    tri.V2 = GetMeshVertex(i2, geometry.VertexBufferIndex);
    return tri;
}

// Object space
// -----------------------------------------------------------------------
Vertex GetIntersectionPointOS(Triangle tri, float3 bary)
{
    Vertex vertex;
    vertex.TexCoord = bary.x * tri.V0.TexCoord + bary.y * tri.V1.TexCoord + bary.z * tri.V2.TexCoord;
    vertex.Normal = bary.x * tri.V0.Normal + bary.y * tri.V1.Normal + bary.z * tri.V2.Normal;
    vertex.Tangent = bary.x * tri.V0.Tangent + bary.y * tri.V1.Tangent + bary.z * tri.V2.Tangent;
    vertex.Bitangent = bary.x * tri.V0.Bitangent + bary.y * tri.V1.Bitangent + bary.z * tri.V2.Bitangent;
    return vertex;
}

// World space
// -----------------------------------------------------------------------
Vertex GetIntersectionPointWS(Vertex vertex)
{
    float3x3 objectToWorld = (float3x3) ObjectToWorld4x3();
    
    Vertex ws;
    ws.Position = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    ws.Normal = normalize(mul(objectToWorld, vertex.Normal));
    ws.Tangent = normalize(mul(objectToWorld, vertex.Tangent));
    ws.Bitangent = normalize(mul(objectToWorld, vertex.Bitangent));
    return ws;
}

#endif // __BINDLESS_RESOURCES_HLSLI__