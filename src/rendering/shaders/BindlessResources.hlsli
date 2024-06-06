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

struct Vertex
{
    float3 Position;
    float2 TexCoord;
    float3 Normal;
    float3 Tangent;
    float3 Bitangent;
};

static const uint c_VertexStructSize = 56;

struct SceneConstants
{
    matrix ViewMatrix;
    matrix ProjectionMatrix;
    matrix InvViewProjMatrix;
    float3 CameraPosition;
    uint NumLights;
};

static const uint c_SceneConstantsStructSize = 208;

struct MaterialConstants
{
    float4 AlbedoColor;
    float Roughness;
    float Metalness;
    uint AlbedoMapIndex;
    uint NormalMapIndex;
    uint RoughnessMapIndex;
    uint MetalnessMapIndex;
};

static const uint c_MaterialConstantsStructSize = 36;

struct GeometryConstants
{
    uint MaterialIndex;
    uint VertexBufferIndex;
    uint IndexBufferIndex;
};

static const uint c_GeometryConstantsStructSize = 12;

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

// Helper functions for loading elements from raw buffers
uint GetMeshIndex(uint i, uint indexBufferIndex)
{
    return g_Buffers[indexBufferIndex].Load(i * 4);
}

Vertex GetMeshVertex(uint i, uint vertexBufferIndex)
{
    return g_Buffers[vertexBufferIndex].Load<Vertex>(i * c_VertexStructSize);
}

MaterialConstants GetMeshMaterial(uint i, uint materialBufferIndex)
{
    return g_Buffers[materialBufferIndex].Load<MaterialConstants>(i * c_MaterialConstantsStructSize);
}

GeometryConstants GetMesh(uint i, uint geometryBufferIndex)
{
    return g_Buffers[geometryBufferIndex].Load<GeometryConstants>(i * c_GeometryConstantsStructSize);
}

Light GetLight(uint i, uint lightsBufferIndex)
{
    return g_Buffers[lightsBufferIndex].Load<Light>(i * c_LightStructSize);
}

#endif // __BINDLESS_RESOURCES_HLSLI__