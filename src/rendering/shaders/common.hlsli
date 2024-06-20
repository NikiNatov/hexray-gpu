#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

#define DIRECTIONAL_LIGHT 0
#define POINT_LIGHT       1
#define SPOT_LIGHT        2
#define AREA_LIGHT        3 // todo

#define MAX_RAY_RECURSION_DEPTH 3

static const float PI = 3.141592;
static const float TwoPI = 2 * PI;
static const float Epsilon = 0.00001;

struct Ray
{
    float3 Origin;
    float3 Direction;
};

struct RayPayload
{
    float4 Color;
    uint RecursionDepth;
};

struct ShadowRayPayload
{
    bool Hit;
};

struct SampleGradValues
{
    float2 TexCoord;
    float2 Ddx;
    float2 Ddy;
};

void GenerateCameraRay(SceneConstants sceneConstants, uint2 index, inout float3 origin, inout float3 direction)
{
    float2 screenPos = (index + 0.5) / (float2) DispatchRaysDimensions().xy * 2.0 - 1.0; // Clip Space [-1, 1]
    screenPos.y = -screenPos.y; // invert y
    
    float4 positionVS = mul(sceneConstants.InvProjMatrix, float4(screenPos, 0.0, 1.0)); // To View Space
    float3 rayDirectionWS = mul(sceneConstants.InvViewMatrix, float4(normalize(positionVS.xyz / positionVS.w), 0.0)).xyz; // To World Space

    origin = sceneConstants.CameraPosition;
    direction = rayDirectionWS;
}

float4 TraceRadianceRay(in Ray ray, in uint currentRayRecursionDepth)
{
    if (currentRayRecursionDepth >= MAX_RAY_RECURSION_DEPTH)
    {
        return float4(0, 0, 0, 0);
    }

    RayDesc rayDesc;
    rayDesc.Origin = ray.Origin;
    rayDesc.Direction = ray.Direction;
    rayDesc.TMin = 0.01;
    rayDesc.TMax = 1000;
    
    RaytracingAccelerationStructure accelerationStructure = g_AccelerationStructures[g_ResourceIndices.AccelerationStructureIndex];
    RayPayload payload = { float4(0, 0, 0, 0), currentRayRecursionDepth + 1 };
    
    TraceRay(accelerationStructure, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 0, 0, rayDesc, payload);

    return payload.Color;
}

bool TraceShadowRay(in Ray ray, in uint currentRayRecursionDepth)
{
    if (currentRayRecursionDepth >= MAX_RAY_RECURSION_DEPTH)
    {
        return false;
    }

    RayDesc rayDesc;
    rayDesc.Origin = ray.Origin;
    rayDesc.Direction = ray.Direction;
    rayDesc.TMin = 0.01;
    rayDesc.TMax = 1000;
    
    RaytracingAccelerationStructure accelerationStructure = g_AccelerationStructures[g_ResourceIndices.AccelerationStructureIndex];
    ShadowRayPayload payload = { true };
    
    TraceRay(accelerationStructure, RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, ~0, 0, 0, 1, rayDesc, payload);

    return payload.Hit;
}

float4 SampleTextureGrad(Texture2D<float4> texture, SamplerState samplerState, SampleGradValues sampleValues)
{
    //return texture.SampleLevel(samplerState, sampleValues.TexCoord, 0);
    return texture.SampleGrad(samplerState, sampleValues.TexCoord, sampleValues.Ddx, sampleValues.Ddy);
}

float3 GetNormalFromMap(Texture2D<float4> normalMap, Vertex vertex, SampleGradValues sampleValues)
{
    // Note: If using DXT5n compression, different unpacking will be required!
    float3x3 TBNMatrix = float3x3(vertex.Tangent, vertex.Bitangent, vertex.Normal);
    float3 normalMapValue = SampleTextureGrad(normalMap, g_LinearClampSampler, sampleValues).rgb * 2.0 - 1.0;
    return normalize(mul(normalMapValue, TBNMatrix));
}

float3 RayPlaneIntersection(float3 planeOrigin, float3 planeNormal, float3 rayOrigin, float3 rayDirection)
{
    float t = dot(-planeNormal, rayOrigin - planeOrigin) / dot(planeNormal, rayDirection);
    return rayOrigin + rayDirection * t;
}

// From "Real-Time Collision Detection" by Christer Ericson
float3 ComputeBarycentricCoordinates(float3 pt, float3 v0, float3 v1, float3 v2)
{
    float3 e0 = v1 - v0;
    float3 e1 = v2 - v0;
    float3 e2 = pt - v0;
    float d00 = dot(e0, e0);
    float d01 = dot(e0, e1);
    float d11 = dot(e1, e1);
    float d20 = dot(e2, e0);
    float d21 = dot(e2, e1);
    float denom = 1.0 / (d00 * d11 - d01 * d01);
    float v = (d11 * d20 - d01 * d21) * denom;
    float w = (d00 * d21 - d01 * d20) * denom;
    float u = 1.0 - v - w;
    return float3(u, v, w);
}


SampleGradValues GetSamplingValues(SceneConstants sceneConstants, Triangle tri, float3 worldPosition, float3 worldNormal, float2 texCoord)
{
    // Better to implement this paper:
    // https://media.contentapi.ea.com/content/dam/ea/seed/presentations/2019-ray-tracing-gems-chapter-20-akenine-moller-et-al.pdf
    //---------------------------------------------------------------------------------------------
    // Compute partial derivatives of UV coordinates:
    //
    //  1) Intersect two helper rays with the plane:  one to the right and one down
    //  2) Compute barycentric coordinates of the two hit points
    //  3) Reconstruct the UV coordinates at the hit points
    //  4) Take the difference in UV coordinates as the partial derivatives X and Y

    // TODO: This is not optimal
    float3x3 objectToWorld = (float3x3) ObjectToWorld4x3();
    float3 P0 = mul(objectToWorld, tri.V0.Position);
    float3 P1 = mul(objectToWorld, tri.V1.Position);
    float3 P2 = mul(objectToWorld, tri.V2.Position);
    
    // Helper rays
    uint2 threadID = DispatchRaysIndex().xy;
    float3 ddxOrigin, ddxDir;
    GenerateCameraRay(sceneConstants, uint2(threadID.x + 1, threadID.y), ddxOrigin, ddxDir);
    float3 ddyOrigin, ddyDir;
    GenerateCameraRay(sceneConstants, uint2(threadID.x, threadID.y + 1), ddyOrigin, ddyDir);

    // Intersect helper rays
    float3 xOffsetPoint = RayPlaneIntersection(worldPosition, worldNormal, ddxOrigin, ddxDir);
    float3 yOffsetPoint = RayPlaneIntersection(worldPosition, worldNormal, ddyOrigin, ddyDir);

    // Compute barycentrics 
    float3 baryX = ComputeBarycentricCoordinates(xOffsetPoint, P0, P1, P2);
    float3 baryY = ComputeBarycentricCoordinates(yOffsetPoint, P0, P1, P2);

    // Compute UVs and take the difference
    float3x2 uvMat = float3x2(tri.V0.TexCoord, tri.V1.TexCoord, tri.V2.TexCoord);
    
    float2 ddx = mul(baryX, uvMat) - texCoord;
    float2 ddy = mul(baryY, uvMat) - texCoord;
    
    SampleGradValues result;
    result.TexCoord = texCoord;
    result.Ddx = ddx;
    result.Ddy = ddy;
    return result;
}

#endif // __COMMON_HLSLI__