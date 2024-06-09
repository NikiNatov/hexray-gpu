#pragma once
#include "directx12.h"

struct TextureDescription
{
    DXGI_FORMAT Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    D3D12_RESOURCE_DIMENSION Type = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    uint32_t Width = 1;
    uint32_t Height = 1;
    uint32_t Depth = 1;
    uint32_t MipLevels = 1;
    uint32_t ArrayLevels = 1;
    D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE;
    D3D12_CLEAR_VALUE ClearValue = {};
    D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON;
    bool IsCubeMap = false;
};
