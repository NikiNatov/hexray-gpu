#pragma once

#include "core/core.h"
#include "rendering/descriptorheap.h"
#include "directx12.h"

struct TextureDescription
{
    DXGI_FORMAT Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    uint32_t Width = 1;
    uint32_t Height = 1;
    uint32_t MipLevels = 0;
    D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE;
    D3D12_CLEAR_VALUE ClearValue = {};
    D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON;
    bool IsCubeMap = false;
};

class Texture
{
public:
    Texture(const TextureDescription& description, const wchar_t* debugName = L"Unnamed Texture");
    ~Texture();

    DescriptorIndex GetSRV();
    DescriptorIndex GetRTV(uint32_t mip);
    DescriptorIndex GetDSV(uint32_t mip);
    DescriptorIndex GetUAV(uint32_t mip);

    inline DXGI_FORMAT GetFormat() const { return m_Description.Format; }
    inline uint32_t GetWidth() const { return m_Description.Width; }
    inline uint32_t GetHeight() const { return m_Description.Height; }
    inline uint32_t GetMipLevels() const { return m_Description.MipLevels; }
    inline D3D12_RESOURCE_FLAGS GetFlags() const { return m_Description.Flags; }
    inline const D3D12_CLEAR_VALUE& GetClearValue() const { return m_Description.ClearValue; }
    inline D3D12_RESOURCE_STATES GetInitialState() const { return m_Description.InitialState; }
    inline bool IsCubeMap() const { return m_Description.IsCubeMap; }
    inline const ComPtr<ID3D12Resource2>& GetResource() const { return m_Resource; }
public:
    static uint32_t CalculateMaxMipCount(uint32_t width, uint32_t height);
private:
    void CreateViews();
private:
    TextureDescription m_Description;
    ComPtr<ID3D12Resource2> m_Resource;
    DescriptorIndex m_SRVDescriptor;
    std::vector<DescriptorIndex> m_MipUAVDescriptors;
    std::vector<DescriptorIndex> m_MipRTVDescriptors;
    std::vector<DescriptorIndex> m_MipDSVDescriptors;
};