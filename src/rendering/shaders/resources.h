#ifndef __BINDLESS_RESOURCES_H__
#define __BINDLESS_RESOURCES_H__

#ifndef HLSL
#include <glm.hpp>
typedef glm::vec2 float2;
typedef glm::vec3 float3;
typedef glm::vec4 float4;
typedef glm::mat4 matrix;
typedef uint32_t uint;
#endif // HLSL

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
static const uint c_IndexBufferElementSize = 4;

// -----------------------------------------------------------------------
struct Triangle
{
    Vertex V0;
    Vertex V1;
    Vertex V2;
};

// -----------------------------------------------------------------------
struct SceneConstants
{
    matrix ViewMatrix;
    matrix ProjectionMatrix;
    matrix InvProjMatrix;
    matrix InvViewMatrix;
    float3 CameraPosition;
    uint NumLights;
    uint FrameIndex;
};

static const uint c_SceneConstantsStructSize = 276;

// -----------------------------------------------------------------------
enum MaterialType
{
    Lambert = 0,
    Phong = 1,
    PBR = 2
};

struct MaterialConstants
{
    uint MaterialType;
    float4 AlbedoColor;
    float4 EmissiveColor;
    float EmissivePower;
    float4 SpecularColor;   // Phong
    float Shininess;        // Phong
    float Roughness;        // PBR
    float Metalness;        // PBR
    uint AlbedoMapIndex;
    uint NormalMapIndex;
    uint RoughnessMapIndex; // PBR
    uint MetalnessMapIndex; // PBR
};

static const uint c_MaterialConstantsStructSize = 84;

// -----------------------------------------------------------------------
struct GeometryConstants
{
    uint MaterialIndex;
    uint VertexBufferIndex;
    uint IndexBufferIndex;
};

static const uint c_GeometryConstantsStructSize = 12;

// -----------------------------------------------------------------------
enum LightType
{
    DirLight = 0,
    PointLight = 1,
    SpotLight = 2
};

struct Light
{
    uint LightType;
    float3 Position;
    float3 Direction;
    float3 Color;
    float Intensity;
    float ConeAngle;
    float3 AttenuationFactors;
};

static const uint c_LightStructSize = 60;

// -----------------------------------------------------------------------
// ---------------------------- Post FX ----------------------------------
// -----------------------------------------------------------------------
struct BloomDownsampleConstants
{
    float SourceMipLevel;
    float DownsampledMipLevel;
    uint SourceTextureIndex;
    uint DownsampledTextureIndex;
};

// -----------------------------------------------------------------------
struct BloomUpsampleConstants
{
    float FilterRadius;
    float SourceMipLevel;
    uint SourceTextureIndex;
    uint UpsampledTextureIndex;
};

// -----------------------------------------------------------------------
struct BloomCompositeConstants
{
    float BloomStrength;
    uint SceneTextureIndex;
    uint BloomTextureIndex;
    uint OutputTextureIndex;
};

struct TonemapConstants
{
    float CameraExposure;
    float Gamma;
    uint InputTextureIndex;
    uint OutputTextureIndex;
};

// -----------------------------------------------------------------------
struct ResourceBindTable
{
    uint EnvironmentMapIndex;
    uint RenderTargetIndex;
    uint PrevFrameRenderTargetIndex;
    uint SceneBufferIndex;
    uint LightsBufferIndex;
    uint MaterialBufferIndex;
    uint GeometryBufferIndex;
    uint AccelerationStructureIndex;

    // PostFX constants
    BloomDownsampleConstants BloomDownsampleConstants;
    BloomUpsampleConstants BloomUpsampleConstants;
    BloomCompositeConstants BloomCompositeConstants;

    TonemapConstants TonemapConstants;
};

// -----------------------------------------------------------------------
struct ColorRayPayload
{
    uint Seed;
    uint RayDepth;
    float4 Color;
};

// -----------------------------------------------------------------------
struct ShadowRayPayload
{
    bool Visible;
};

// -----------------------------------------------------------------------
struct HitInfo
{
    float2 TexCoords;
    float3 WorldPosition;
    float3 WorldNormal;
    float3 WorldTangent;
    float3 WorldBitangent;
};

static const float PI = 3.141592;
static const float TwoPI = 2 * PI;
static const float Epsilon = 0.000001;

#ifdef HLSL
#include "common.hlsli"

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
#define MAX_RAY_RECURSION_DEPTH 4

ConstantBuffer<ResourceBindTable> g_ResourceIndices          : register(b0, RESOURCE_INDICES_SPACE);
RaytracingAccelerationStructure   g_AccelerationStructures[] : register(t0, ACCELERATION_STRUCTURES_SPACE);
ByteAddressBuffer                 g_Buffers[]                : register(t0, ROBUFFERS_SPACE);
Texture2D<float4>                 g_Textures[]               : register(t0, ROTEXTURE2D_SPACE);
TextureCube                       g_CubeMaps[]               : register(t0, ROTEXTURECUBE_SPACE);
RWTexture2D<float4>               g_RWTextures[]             : register(u0, RWTEXTURE2D_SPACE);
SamplerState                      g_PointClampSampler        : register(s0, SAMPLERSTATE_SPACE);
SamplerState                      g_PointWrapSampler         : register(s1, SAMPLERSTATE_SPACE);
SamplerState                      g_LinearClampSampler       : register(s2, SAMPLERSTATE_SPACE);
SamplerState                      g_LinearWrapSampler        : register(s3, SAMPLERSTATE_SPACE);
SamplerState                      g_AnisoWrapSampler         : register(s4, SAMPLERSTATE_SPACE);

// -----------------------------------------------------------------------
// Helper functions for tracing rays
// -----------------------------------------------------------------------
ColorRayPayload TraceColorRay(float3 origin, float3 direction, uint seed, uint currentRayDepth, RaytracingAccelerationStructure accelerationStructure)
{
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = 0.001;
    ray.TMax = 1000.0;

    ColorRayPayload payload;
    payload.Seed = seed;
    payload.RayDepth = currentRayDepth + 1;
    payload.Color = float4(0.0, 0.0, 0.0, 1.0);

    if (payload.RayDepth > MAX_RAY_RECURSION_DEPTH)
    {
        return payload;
    }

    TraceRay(accelerationStructure, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 0, 0, ray, payload);
    return payload;
}

// -----------------------------------------------------------------------
bool TraceShadowRay(float3 origin, float3 direction, float maxDistance, RaytracingAccelerationStructure accelerationStructure)
{
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = 0.001;
    ray.TMax = maxDistance;

    ShadowRayPayload payload;
    payload.Visible = false;

    TraceRay(accelerationStructure, RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, ~0, 0, 0, 1, ray, payload);
    return payload.Visible;
}

// -----------------------------------------------------------------------
// Helper functions for loading elements from raw buffers
// -----------------------------------------------------------------------
uint GetMeshIndex(uint i, uint indexBufferIndex)
{
    return g_Buffers[indexBufferIndex].Load(i * c_IndexBufferElementSize);
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

// -----------------------------------------------------------------------
float3 GetNormalFromMap(Texture2D<float4> normalMap, HitInfo hitInfo, uint mipLevel)
{
    // Note: If using DXT5n compression, different unpacking will be required!
    float3x3 TBNMatrix = float3x3(hitInfo.WorldTangent, hitInfo.WorldBitangent, hitInfo.WorldNormal);
    float3 normalMapValue = normalMap.SampleLevel(g_LinearClampSampler, hitInfo.TexCoords, mipLevel).rgb * 2.0 - 1.0;
    return normalize(mul(normalMapValue, TBNMatrix));
}

// -----------------------------------------------------------------------
HitInfo GetHitInfo(BuiltInTriangleIntersectionAttributes attr)
{
    uint geometryID = InstanceID();
    uint primitiveID = PrimitiveIndex();

    GeometryConstants geometry = GetMesh(geometryID, g_ResourceIndices.GeometryBufferIndex);
    Triangle tri = GetTriangle(geometry, primitiveID);
    Vertex ip = GetIntersectionPointOS(tri, float3(1.0 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y));

    HitInfo hitInfo;
    hitInfo.TexCoords = ip.TexCoord;
    hitInfo.WorldPosition = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    hitInfo.WorldNormal = normalize(mul((float3x3)ObjectToWorld4x3(), ip.Normal));
    hitInfo.WorldTangent = normalize(mul((float3x3)ObjectToWorld4x3(), ip.Tangent));
    hitInfo.WorldBitangent = normalize(mul((float3x3)ObjectToWorld4x3(), ip.Bitangent));

    return hitInfo;
}
#endif // HLSL

#endif // __BINDLESS_RESOURCES_H__