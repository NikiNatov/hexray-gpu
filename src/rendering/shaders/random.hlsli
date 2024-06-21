#ifndef __RANDOM_HLSLI__
#define __RANDOM_HLSLI__

#include "resources.h"

uint GenerateRandomSeed(uint rayIndex, uint frameIndex)
{
    uint s0 = 0;

	[unroll]
    for (uint n = 0; n < 16; n++)
    {
        s0 += 0x9e3779b9;
        rayIndex += ((frameIndex << 4) + 0xa341316c) ^ (frameIndex + s0) ^ ((frameIndex >> 5) + 0xc8013ea4);
        frameIndex += ((rayIndex << 4) + 0xad90777d) ^ (rayIndex + s0) ^ ((rayIndex >> 5) + 0x7e95761e);
    }
    return rayIndex;
}

float RandomFloat(inout uint s)
{
    s = (1664525u * s + 1013904223u);
    return float(s & 0x00FFFFFF) / float(0x01000000);
}

float3 GetRandomDirectionCosineWeighted(inout uint seed, float3 normal, float3 tangent, float3 bitangent)
{
	// Get our uniform random numbers
    float2 randValues = float2(RandomFloat(seed), RandomFloat(seed));
    
    // Cosine weighted sampling
    // https://www.mathematik.uni-marburg.de/~thormae/lectures/graphics1/code/ImportanceSampling/importance_sampling_notes.pdf
    float sinTheta = sqrt(randValues.x);
    float cosTheta = sqrt(1.0 - sinTheta * sinTheta);
    float phi = 2.0 * PI * randValues.y;
    
	// Get our Cosine weighted sample direction
    float3 microfacetNormal = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
    return normalize(tangent * microfacetNormal.x + bitangent * microfacetNormal.y + normal * microfacetNormal.z);
}

float3 GetRandomDirectionGGX(inout uint seed, float roughness, float3 normal, float3 tangent, float3 bitangent)
{
	// Get our uniform random numbers
    float2 randValues = float2(RandomFloat(seed), RandomFloat(seed));

	// GGX normal distribution function sampling
    // https://www.mathematik.uni-marburg.de/~thormae/lectures/graphics1/code/ImportanceSampling/importance_sampling_notes.pdf
    float alpha = roughness * roughness;
    float cosTheta = sqrt((1.0 - randValues.x) / ((alpha - 1.0) * randValues.x + 1.0));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
    float phi = 2.0 * PI * randValues.y;

	// Get our GGX normal distribution function sample direction
    float3 microfacetNormal = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
    return normalize(tangent * microfacetNormal.x + bitangent * microfacetNormal.y + normal * microfacetNormal.z);
}

float3 GetRandomDirectionBlinnPhong(inout uint seed, float shininess, float3 normal, float3 tangent, float3 bitangent)
{
	// Get our uniform random numbers
    float2 randValues = float2(RandomFloat(seed), RandomFloat(seed));

	// Blinn-Phong distribution function sampling
    // https://www.mathematik.uni-marburg.de/~thormae/lectures/graphics1/code/ImportanceSampling/importance_sampling_notes.pdf
    float cosTheta = pow(1.0 - randValues.x, 1.0 / (shininess + 1.0));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
    float phi = 2.0 * PI * randValues.y;

	// Get our Blinn-Phong distribution function sample direction
    float3 microfacetNormal = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
    return normalize(tangent * microfacetNormal.x + bitangent * microfacetNormal.y + normal * microfacetNormal.z);
}

#endif // __RANDOM_HLSLI__