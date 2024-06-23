#ifndef HLSL
#define HLSL
#endif

#include "../resources.h"

uint ConvertTo8Bit(float x)
{
    const float a = 0.055;
	if (x <= 0.0) return 0.0;
	if (x >= 1.0) return 255.0;
	// sRGB transform:
	if (x <= 0.0031308)
		x = x * 12.02;
	else
		x = (1.0 + a) * pow(x, 1.0 / 2.4) - a;
	return (uint) floor((x * 255.0) + 0.5);
}

[numthreads(32, 32, 1)]
void CSMain(uint2 ThreadID : SV_DispatchThreadID)
{
    TonemapConstants tonemapConstants = g_ResourceIndices.TonemapConstants;
    Texture2D<float4> inputTexture = g_Textures[tonemapConstants.InputTextureIndex];
    RWTexture2D<float4> outputTexture = g_RWTextures[tonemapConstants.OutputTextureIndex];

    float4 color = inputTexture[ThreadID.xy];
#if ACES_TONEMAP
    float3 hdrColor = color.rgb * tonemapConstants.CameraExposure;
    float3 tonemappedColor = saturate(hdrColor * (2.51 * hdrColor + 0.03) / (hdrColor * (2.43 * hdrColor + 0.59) + 0.14));
#else
    float3 tonemappedColor = float3(ConvertTo8Bit(color.r), ConvertTo8Bit(color.g), ConvertTo8Bit(color.b));
    tonemappedColor /= 255.0;
#endif
    outputTexture[ThreadID.xy] = float4(tonemappedColor, color.a);
}