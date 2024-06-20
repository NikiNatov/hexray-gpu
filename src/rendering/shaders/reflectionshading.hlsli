#ifndef __REFLECTIONSHADING_HLSLI__
#define __REFLECTIONSHADING_HLSLI__

#include "BindlessResources.hlsli"

float2 RandomUnitDiskSample(uint sampleNumber)
{
    float2 poissonDisk[64];
    poissonDisk[0] = float2(-0.613392, 0.617481);
    poissonDisk[1] = float2(0.170019, -0.040254);
    poissonDisk[2] = float2(-0.299417, 0.791925);
    poissonDisk[3] = float2(0.645680, 0.493210);
    poissonDisk[4] = float2(-0.651784, 0.717887);
    poissonDisk[5] = float2(0.421003, 0.027070);
    poissonDisk[6] = float2(-0.817194, -0.271096);
    poissonDisk[7] = float2(-0.705374, -0.668203);
    poissonDisk[8] = float2(0.977050, -0.108615);
    poissonDisk[9] = float2(0.063326, 0.142369);
    poissonDisk[10] = float2(0.203528, 0.214331);
    poissonDisk[11] = float2(-0.667531, 0.326090);
    poissonDisk[12] = float2(-0.098422, -0.295755);
    poissonDisk[13] = float2(-0.885922, 0.215369);
    poissonDisk[14] = float2(0.566637, 0.605213);
    poissonDisk[15] = float2(0.039766, -0.396100);
    poissonDisk[16] = float2(0.751946, 0.453352);
    poissonDisk[17] = float2(0.078707, -0.715323);
    poissonDisk[18] = float2(-0.075838, -0.529344);
    poissonDisk[19] = float2(0.724479, -0.580798);
    poissonDisk[20] = float2(0.222999, -0.215125);
    poissonDisk[21] = float2(-0.467574, -0.405438);
    poissonDisk[22] = float2(-0.248268, -0.814753);
    poissonDisk[23] = float2(0.354411, -0.887570);
    poissonDisk[24] = float2(0.175817, 0.382366);
    poissonDisk[25] = float2(0.487472, -0.063082);
    poissonDisk[26] = float2(-0.084078, 0.898312);
    poissonDisk[27] = float2(0.488876, -0.783441);
    poissonDisk[28] = float2(0.470016, 0.217933);
    poissonDisk[29] = float2(-0.696890, -0.549791);
    poissonDisk[30] = float2(-0.149693, 0.605762);
    poissonDisk[31] = float2(0.034211, 0.979980);
    poissonDisk[32] = float2(0.503098, -0.308878);
    poissonDisk[33] = float2(-0.016205, -0.872921);
    poissonDisk[34] = float2(0.385784, -0.393902);
    poissonDisk[35] = float2(-0.146886, -0.859249);
    poissonDisk[36] = float2(0.643361, 0.164098);
    poissonDisk[37] = float2(0.634388, -0.049471);
    poissonDisk[38] = float2(-0.688894, 0.007843);
    poissonDisk[39] = float2(0.464034, -0.188818);
    poissonDisk[40] = float2(-0.440840, 0.137486);
    poissonDisk[41] = float2(0.364483, 0.511704);
    poissonDisk[42] = float2(0.034028, 0.325968);
    poissonDisk[43] = float2(0.099094, -0.308023);
    poissonDisk[44] = float2(0.693960, -0.366253);
    poissonDisk[45] = float2(0.678884, -0.204688);
    poissonDisk[46] = float2(0.001801, 0.780328);
    poissonDisk[47] = float2(0.145177, -0.898984);
    poissonDisk[48] = float2(0.062655, -0.611866);
    poissonDisk[49] = float2(0.315226, -0.604297);
    poissonDisk[50] = float2(-0.780145, 0.486251);
    poissonDisk[51] = float2(-0.371868, 0.882138);
    poissonDisk[52] = float2(0.200476, 0.494430);
    poissonDisk[53] = float2(-0.494552, -0.711051);
    poissonDisk[54] = float2(0.612476, 0.705252);
    poissonDisk[55] = float2(-0.578845, -0.768792);
    poissonDisk[56] = float2(-0.772454, -0.090976);
    poissonDisk[57] = float2(0.504440, 0.372295);
    poissonDisk[58] = float2(0.155736, 0.065157);
    poissonDisk[59] = float2(0.391522, 0.849605);
    poissonDisk[60] = float2(-0.620106, -0.328104);
    poissonDisk[61] = float2(0.789239, -0.419965);
    poissonDisk[62] = float2(-0.545396, 0.538133);
    poissonDisk[63] = float2(-0.178564, -0.596057);
    return poissonDisk[sampleNumber % 64];
}

void CreateOrthonormalBasis(float3 vec, inout float3 a, inout float3 b)
{
    float3 kTestVectorA = float3(0.577350269, 0.577350269, 0.577350269);
    if (abs(dot(vec, kTestVectorA)) <= 0.9)
    {
        a = cross(kTestVectorA, vec);
    }
    else
    {
        float3 kTestVectorB = float3(0.267261242, -0.534522484, 0.801783726);
        a = cross(kTestVectorB, vec);
    }
	
    a = normalize(a);
    b = cross(a, vec);
}

[shader("closesthit")]
void ClosestHitShader_Reflection(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    if (payload.Depth >= MAX_RECURSION_DEPTH)
    {
        return;
    }

    uint geometryID = InstanceID();
    uint triangleIndex = PrimitiveIndex();
    
    RaytracingAccelerationStructure accelerationStructure = g_AccelerationStructures[g_ResourceIndices.AccelerationStructureIndex];
    MaterialConstants material = GetMeshMaterial(geometryID, g_ResourceIndices.MaterialBufferIndex);
    GeometryConstants geometry = GetMesh(geometryID, g_ResourceIndices.GeometryBufferIndex);
    
    //payload.Color = float4(material.ReflectionColor, 1.0);
    //return;
    float3 bary = float3(1.0 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    Triangle tri = GetTriangle(geometry, triangleIndex);
    Vertex ip = GetIntersectionPointVS(tri, bary);
    
    float3 surfacePositionWS = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float3 normalWS = normalize(mul((float3x3) ObjectToWorld4x3(), ip.Normal));
    
    RayDesc newRay;
    newRay.Origin = surfacePositionWS + normalWS * 1e-6;
    newRay.TMin = 0.001;
    newRay.TMax = 1000.0;
    
    RayPayload reflectionPayload;
    reflectionPayload.Color = float4(0, 0, 0, 0);
    reflectionPayload.Depth = payload.Depth + 1;

    //if (material.Glossiness < 1.0f)
    //{
    //    float scaling = pow(10.0f, 2 - 8 * material.Glossiness);
    //    float3 u, v;
    //    CreateOrthonormalBasis(normalWS, u, v);
    //    
    //    float4 color = float4(0, 0, 0, 0);
    //    for (uint i = 0; i < material.NumberOfSamples; i++)
    //    {
    //        float3 reflected;
    //        do
    //        {
    //            float2 unitDiskSample = RandomUnitDiskSample(i);
    //            unitDiskSample *= scaling;
    //
    //            float3 modifiedNormal = normalize(normalWS + u * unitDiskSample.x + v * unitDiskSample.y);
    //            reflected = reflect(WorldRayDirection(), modifiedNormal);
    //        }
    //        while (dot(reflected, normalWS) < 0.0);
    //        
    //        newRay.Direction = reflected;
    //        
    //        TraceRay(accelerationStructure, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, newRay, reflectionPayload);
    //
    //        color += reflectionPayload.Color * float4(material.ReflectionColor, 1.0);
    //    }
    //    
    //    payload.Color = color / float(material.NumberOfSamples);
    //    return;
    //}

    newRay.Direction = reflect(WorldRayDirection(), normalWS);

    //TraceRay(accelerationStructure, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, newRay, reflectionPayload);
    payload.Color = reflectionPayload.Color * float4(material.ReflectionColor, 1.0);
}

#endif // __REFLECTIONSHADING_HLSLI__