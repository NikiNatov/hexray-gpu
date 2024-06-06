#pragma once

#include "core/core.h"
#include "directx12.h"

struct TextureDescription;

uint32_t CalculateRowPitch(uint32_t width, DXGI_FORMAT format);
uint32_t CalculateSlicePitch(uint32_t width, uint32_t height, DXGI_FORMAT format);
uint32_t CalculateMipOffset(const TextureDescription& desc, uint32_t arrayLevel, uint32_t mip);
uint32_t CalculateTextureSize(const TextureDescription& desc);
uint32_t CalculateMaxMipCount(uint32_t width, uint32_t height);