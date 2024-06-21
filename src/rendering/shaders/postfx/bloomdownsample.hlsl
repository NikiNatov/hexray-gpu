#ifndef HLSL
#define HLSL
#endif

#include "../resources.h"

[numthreads(32, 32, 1)]
void CSMain(uint2 ThreadID : SV_DispatchThreadID)
{
    BloomDownsampleConstants downsampleConstants = g_ResourceIndices.BloomDownsampleConstants;
    RWTexture2D<float4> downsampledTexture = g_RWTextures[downsampleConstants.DownsampledTextureIndex];
    Texture2D<float4> sourceTexture = g_Textures[downsampleConstants.SourceTextureIndex];
    
    float targetWidth, targetHeight;
    downsampledTexture.GetDimensions(targetWidth, targetHeight);
    
    float2 targetTexelSize = 1.0f / float2(targetWidth, targetHeight);
    float2 texCoords = targetTexelSize * (ThreadID.xy);
    
    if (downsampleConstants.DownsampledMipLevel == 0)
    {
        downsampledTexture[ThreadID.xy] = sourceTexture.SampleLevel(g_LinearClampSampler, texCoords, downsampleConstants.SourceMipLevel);
        return;
    }

    // Down sampling is done based on the method used in "Next-gen post processing in Call of Duty" presentation, ACM Siggraph 2014
    // https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/
	// a - b - c
	// - j - k -
	// d - e - f
	// - l - m -
	// g - h - i
	// 'e' = current texel
    
    float3 a = sourceTexture.SampleLevel(g_LinearClampSampler, float2(texCoords.x - 2 * targetTexelSize.x, texCoords.y + 2 * targetTexelSize.y), downsampleConstants.SourceMipLevel).rgb;
    float3 b = sourceTexture.SampleLevel(g_LinearClampSampler, float2(texCoords.x,                         texCoords.y + 2 * targetTexelSize.y), downsampleConstants.SourceMipLevel).rgb;
    float3 c = sourceTexture.SampleLevel(g_LinearClampSampler, float2(texCoords.x + 2 * targetTexelSize.x, texCoords.y + 2 * targetTexelSize.y), downsampleConstants.SourceMipLevel).rgb;

    float3 d = sourceTexture.SampleLevel(g_LinearClampSampler, float2(texCoords.x - 2 * targetTexelSize.x, texCoords.y), downsampleConstants.SourceMipLevel).rgb;
    float3 e = sourceTexture.SampleLevel(g_LinearClampSampler, float2(texCoords.x,                         texCoords.y), downsampleConstants.SourceMipLevel).rgb;
    float3 f = sourceTexture.SampleLevel(g_LinearClampSampler, float2(texCoords.x + 2 * targetTexelSize.x, texCoords.y), downsampleConstants.SourceMipLevel).rgb;

    float3 g = sourceTexture.SampleLevel(g_LinearClampSampler, float2(texCoords.x - 2 * targetTexelSize.x, texCoords.y - 2 * targetTexelSize.y), downsampleConstants.SourceMipLevel).rgb;
    float3 h = sourceTexture.SampleLevel(g_LinearClampSampler, float2(texCoords.x,                         texCoords.y - 2 * targetTexelSize.y), downsampleConstants.SourceMipLevel).rgb;
    float3 i = sourceTexture.SampleLevel(g_LinearClampSampler, float2(texCoords.x + 2 * targetTexelSize.x, texCoords.y - 2 * targetTexelSize.y), downsampleConstants.SourceMipLevel).rgb;

    float3 j = sourceTexture.SampleLevel(g_LinearClampSampler, float2(texCoords.x - targetTexelSize.x, texCoords.y + targetTexelSize.y), downsampleConstants.SourceMipLevel).rgb;
    float3 k = sourceTexture.SampleLevel(g_LinearClampSampler, float2(texCoords.x + targetTexelSize.x, texCoords.y + targetTexelSize.y), downsampleConstants.SourceMipLevel).rgb;
    float3 l = sourceTexture.SampleLevel(g_LinearClampSampler, float2(texCoords.x - targetTexelSize.x, texCoords.y - targetTexelSize.y), downsampleConstants.SourceMipLevel).rgb;
    float3 m = sourceTexture.SampleLevel(g_LinearClampSampler, float2(texCoords.x + targetTexelSize.x, texCoords.y - targetTexelSize.y), downsampleConstants.SourceMipLevel).rgb;

    float3 downSampledColor = (j + k + m + l) * 0.5f / 4.0f;
    downSampledColor += (a + b + e + d) * 0.125f / 4.0f;
    downSampledColor += (b + c + f + e) * 0.125f / 4.0f;
    downSampledColor += (d + e + h + g) * 0.125f / 4.0f;
    downSampledColor += (e + f + i + h) * 0.125f / 4.0f;
    
    downsampledTexture[ThreadID.xy] = float4(downSampledColor, 1.0f);
}