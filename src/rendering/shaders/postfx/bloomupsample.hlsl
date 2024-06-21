#ifndef HLSL
#define HLSL
#endif

#include "../resources.h"

[numthreads(32, 32, 1)]
void CSMain(uint2 ThreadID : SV_DispatchThreadID)
{
    BloomUpsampleConstants upsampleConstants = g_ResourceIndices.BloomUpsampleConstants;
    RWTexture2D<float4> upsampledTexture = g_RWTextures[upsampleConstants.UpsampledTextureIndex];
    Texture2D<float4> sourceTexture = g_Textures[upsampleConstants.SourceTextureIndex];
    
    float upsampledWidth, upsampledHeight;
    upsampledTexture.GetDimensions(upsampledWidth, upsampledHeight);
    
    float2 upsampleTexelSize = 1.0f / float2(upsampledWidth, upsampledHeight);
    float2 texCoords = upsampleTexelSize * (ThreadID.xy);
    
    // Upsampling is done based on the method used in "Next-gen post processing in Call of Duty" presentation, ACM Siggraph 2014
    // https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/
	// a - b - c
	// d - e - f
	// g - h - i
	// 'e' = current texel
    
    float radius = upsampleConstants.FilterRadius;
    float3 a = sourceTexture.SampleLevel(g_LinearClampSampler, float2(texCoords.x - radius, texCoords.y + radius), upsampleConstants.SourceMipLevel).rgb;
    float3 b = sourceTexture.SampleLevel(g_LinearClampSampler, float2(texCoords.x,          texCoords.y + radius), upsampleConstants.SourceMipLevel).rgb;
    float3 c = sourceTexture.SampleLevel(g_LinearClampSampler, float2(texCoords.x + radius, texCoords.y + radius), upsampleConstants.SourceMipLevel).rgb;
    
    float3 d = sourceTexture.SampleLevel(g_LinearClampSampler, float2(texCoords.x - radius, texCoords.y), upsampleConstants.SourceMipLevel).rgb;
    float3 e = sourceTexture.SampleLevel(g_LinearClampSampler, float2(texCoords.x,          texCoords.y), upsampleConstants.SourceMipLevel).rgb;
    float3 f = sourceTexture.SampleLevel(g_LinearClampSampler, float2(texCoords.x + radius, texCoords.y), upsampleConstants.SourceMipLevel).rgb;
    
    float3 g = sourceTexture.SampleLevel(g_LinearClampSampler, float2(texCoords.x - radius, texCoords.y - radius), upsampleConstants.SourceMipLevel).rgb;
    float3 h = sourceTexture.SampleLevel(g_LinearClampSampler, float2(texCoords.x,          texCoords.y - radius), upsampleConstants.SourceMipLevel).rgb;
    float3 i = sourceTexture.SampleLevel(g_LinearClampSampler, float2(texCoords.x + radius, texCoords.y - radius), upsampleConstants.SourceMipLevel).rgb;
    
    // Apply a 3x3 tent filter:
    float3 upsampledColor = e * 4.0f;
    upsampledColor += (b + d + f + h) * 2.0f;
    upsampledColor += (a + c + g + i);
    upsampledColor *= 1.0f / 16.0f;
    
    upsampledTexture[ThreadID.xy] += float4(upsampledColor, 1.0f);
}