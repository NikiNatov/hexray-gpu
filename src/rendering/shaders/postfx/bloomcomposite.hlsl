#ifndef HLSL
#define HLSL
#endif

#include "../resources.h"

[numthreads(32, 32, 1)]
void CSMain(uint2 ThreadID : SV_DispatchThreadID)
{
    SceneConstants sceneConstants = g_Buffers[g_ResourceIndices.SceneBufferIndex].Load<SceneConstants>(0);
    BloomCompositeConstants compositeConstants = g_ResourceIndices.BloomCompositeConstants;
    RWTexture2D<float4> outputTexture = g_RWTextures[compositeConstants.OutputTextureIndex];
    Texture2D<float4> bloomTexture = g_Textures[compositeConstants.BloomTextureIndex];
    Texture2D<float4> sceneTexture = g_Textures[compositeConstants.SceneTextureIndex];
    
    float width, height;
    outputTexture.GetDimensions(width, height);
    
    float2 texelSize = 1.0f / float2(width, height);
    float2 texCoords = texelSize * ThreadID.xy;
    
    float3 sceneColor = sceneTexture.SampleLevel(g_LinearClampSampler, texCoords, 0).rgb;
    float3 bloomColor = bloomTexture.SampleLevel(g_LinearClampSampler, texCoords, 0).rgb;

    outputTexture[ThreadID.xy] = float4(lerp(sceneColor, bloomColor, compositeConstants.BloomStrength), 1.0);
}