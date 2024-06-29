#ifndef HLSL
#define HLSL
#endif

#include "../resources.h"

[numthreads(32, 32, 1)]
void CSMain(uint2 ThreadID : SV_DispatchThreadID)
{
    BloomCompositeConstants compositeConstants = g_ResourceIndices.BloomCompositeConstants;
    RWTexture2D<float4> outputTexture = g_RWTextures[compositeConstants.OutputTextureIndex];
    Texture2D<float4> bloomTexture = g_Textures[compositeConstants.BloomTextureIndex];
    Texture2D<float4> sceneTexture = g_Textures[compositeConstants.SceneTextureIndex];

    float3 sceneColor = sceneTexture[ThreadID.xy].rgb;
    float3 bloomColor = bloomTexture[ThreadID.xy].rgb;

    outputTexture[ThreadID.xy] = float4(lerp(sceneColor, bloomColor, compositeConstants.BloomStrength), 1.0);
}