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

struct SceneConstants
{
    matrix ViewMatrix;
    matrix ProjectionMatrix;
    matrix InvViewProjMatrix;
    float3 CameraPosition;
    uint NumLights;
};

struct MaterialConstants
{
    float3 AlbedoColor;
    float Roughness;
    float Metalness;
    uint AlbedoMapIndex;
    uint NormalMapIndex;
    uint RoughnessMapIndex;
    uint MetalnessMapIndex;
};

struct GeometryConstants
{
    uint StartIndex;
    uint StartVertex;
    uint MaterialIndex;
};

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

struct ResourceIndices
{
    uint RenderTargetIndex;
    uint SceneBufferIndex;
    uint LightsBufferIndex;
    uint MaterialBufferIndex;
    uint GeometryBufferIndex;
    uint AccelerationStructureIndex;
};

SamplerState                    g_AnisoWrapSampler         : register(s4, SAMPLERSTATE_SPACE);
ConstantBuffer<ResourceIndices> g_ResourceIndices          : register(b0, RESOURCE_INDICES_SPACE);
RaytracingAccelerationStructure g_AccelerationStructures[] : register(t0, ACCELERATION_STRUCTURES_SPACE);
ByteAddressBuffer               g_Buffers[]                : register(t0, ROBUFFERS_SPACE);
Texture2D<float4>               g_Textures[]               : register(t0, ROTEXTURE2D_SPACE);
TextureCube                     g_CubeMaps[]               : register(t0, ROTEXTURECUBE_SPACE);
RWTexture2D<float4>             g_RWTextures[]             : register(u0, RWTEXTURE2D_SPACE);

#endif // __BINDLESS_RESOURCES_HLSLI__