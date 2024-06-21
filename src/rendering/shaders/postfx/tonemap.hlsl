#ifndef HLSL
#define HLSL
#endif

#include "../resources.h"

[numthreads(32, 32, 1)]
void CSMain(uint2 ThreadID : SV_DispatchThreadID)
{
    TonemapConstants tonemapConstants = g_ResourceIndices.TonemapConstants;
    Texture2D<float4> inputTexture = g_Textures[tonemapConstants.InputTextureIndex];
    RWTexture2D<float4> outputTexture = g_RWTextures[tonemapConstants.OutputTextureIndex];
    
    float width, height;
    inputTexture.GetDimensions(width, height);
    
    float2 texelSize = 1.0f / float2(width, height);
    float2 texCoords = texelSize * ThreadID.xy;
    
    float3 hdrColor = inputTexture.SampleLevel(g_LinearClampSampler, texCoords, 0).rgb * tonemapConstants.CameraExposure;
    float3 tonemappedColor = saturate(hdrColor * (2.51 * hdrColor + 0.03) / (hdrColor * (2.43 * hdrColor + 0.59) + 0.14));

    outputTexture[ThreadID.xy] = float4(pow(tonemappedColor, 1.0 / tonemapConstants.Gamma), 1.0);
}