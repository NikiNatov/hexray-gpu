#include "texture.h"
#include "rendering/graphicscontext.h"

#include <cmath>

// ------------------------------------------------------------------------------------------------------------------------------------
Texture::Texture(const TextureDescription& description, const wchar_t* debugName)
    : m_Description(description)
{
    if (m_Description.MipLevels == 0)
        m_Description.MipLevels = CalculateMaxMipCount(m_Description.Width, m_Description.Height);

    auto d3dDevice = GraphicsContext::GetInstance()->GetDevice();

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Format = m_Description.Format;
    resourceDesc.Width = m_Description.Width;
    resourceDesc.Height = m_Description.Height;
    resourceDesc.DepthOrArraySize = m_Description.IsCubeMap ? 6 : 1;
    resourceDesc.MipLevels = m_Description.MipLevels;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDesc.Flags = m_Description.Flags;

    D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_STATES initialState = m_Description.InitialState;

    bool isRenderTargetOrDepthBuffer = (m_Description.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) || (m_Description.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    DXCall(d3dDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, initialState,
        isRenderTargetOrDepthBuffer ? &m_Description.ClearValue : nullptr, IID_PPV_ARGS(&m_Resource)));
    DXCall(m_Resource->SetName(debugName));

    CreateViews();
}

// ------------------------------------------------------------------------------------------------------------------------------------
Texture::~Texture()
{
    if ((m_Description.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
    {
        GraphicsContext::GetInstance()->GetResourceDescriptorHeap()->ReleaseDescriptor(m_SRVDescriptor, true);
    }

    if (m_Description.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
    {
        for (uint32_t mip = 0; mip < m_Description.MipLevels; mip++)
            GraphicsContext::GetInstance()->GetResourceDescriptorHeap()->ReleaseDescriptor(m_MipUAVDescriptors[mip], true);
    }

    if (m_Description.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
    {
        for (uint32_t mip = 0; mip < m_Description.MipLevels; mip++)
            GraphicsContext::GetInstance()->GetRTVDescriptorHeap()->ReleaseDescriptor(m_MipRTVDescriptors[mip], true);
    }

    if (m_Description.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
    {
        for (uint32_t mip = 0; mip < m_Description.MipLevels; mip++)
            GraphicsContext::GetInstance()->GetDSVDescriptorHeap()->ReleaseDescriptor(m_MipDSVDescriptors[mip], true);
    }

    GraphicsContext::GetInstance()->ReleaseResource(m_Resource.Detach(), true);
}

// ------------------------------------------------------------------------------------------------------------------------------------
DescriptorIndex Texture::GetSRV()
{
    if (m_Description.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)
        return InvalidDescriptorIndex;

    return m_SRVDescriptor;
}

// ------------------------------------------------------------------------------------------------------------------------------------
DescriptorIndex Texture::GetRTV(uint32_t mip)
{
    if ((m_Description.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) == 0)
        return InvalidDescriptorIndex;

    return m_MipRTVDescriptors[mip];
}

// ------------------------------------------------------------------------------------------------------------------------------------
DescriptorIndex Texture::GetDSV(uint32_t mip)
{
    if ((m_Description.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) == 0)
        return InvalidDescriptorIndex;

    return m_MipDSVDescriptors[mip];
}

// ------------------------------------------------------------------------------------------------------------------------------------
DescriptorIndex Texture::GetUAV(uint32_t mip)
{
    if ((m_Description.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0)
        return InvalidDescriptorIndex;

    return m_MipUAVDescriptors[mip];
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Texture::CreateViews()
{
    auto d3dDevice = GraphicsContext::GetInstance()->GetDevice();
    SegregatedDescriptorHeap* resourceHeap = GraphicsContext::GetInstance()->GetResourceDescriptorHeap();
    StandardDescriptorHeap* rtvHeap = GraphicsContext::GetInstance()->GetRTVDescriptorHeap();
    StandardDescriptorHeap* dsvHeap = GraphicsContext::GetInstance()->GetDSVDescriptorHeap();

    // SRV
    m_SRVDescriptor = InvalidDescriptorIndex;
    if ((m_Description.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
    {
        DXGI_FORMAT srvFormat = m_Description.Format;

        if (m_Description.Format == DXGI_FORMAT_D24_UNORM_S8_UINT)
        {
            srvFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        }
        else if (m_Description.Format == DXGI_FORMAT_D32_FLOAT)
        {
            srvFormat = DXGI_FORMAT_R32_FLOAT;
        }

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = srvFormat;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        if (m_Description.IsCubeMap)
        {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            srvDesc.TextureCube.MostDetailedMip = 0;
            srvDesc.TextureCube.MipLevels = m_Description.MipLevels;

            m_SRVDescriptor = resourceHeap->Allocate(ROTextureCube);
        }
        else
        {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.MipLevels = m_Description.MipLevels;

            m_SRVDescriptor = resourceHeap->Allocate(ROTexture2D);
        }

        d3dDevice->CreateShaderResourceView(m_Resource.Get(), &srvDesc, resourceHeap->GetCPUHandle(m_SRVDescriptor));
    }

    // UAV
    if (m_Description.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
    {
        DXGI_FORMAT uavFormat = m_Description.Format;

        if (m_Description.Format == DXGI_FORMAT_D24_UNORM_S8_UINT)
        {
            uavFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        }
        else if (m_Description.Format == DXGI_FORMAT_D32_FLOAT)
        {
            uavFormat = DXGI_FORMAT_R32_FLOAT;
        }

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = uavFormat;
        uavDesc.ViewDimension = m_Description.IsCubeMap ? D3D12_UAV_DIMENSION_TEXTURE2DARRAY : D3D12_UAV_DIMENSION_TEXTURE2D;

        m_MipUAVDescriptors.resize(m_Description.MipLevels);

        for (uint32_t mip = 0; mip < m_Description.MipLevels; mip++)
        {
            if (m_Description.IsCubeMap)
            {
                uavDesc.Texture2DArray.MipSlice = mip;
                uavDesc.Texture2DArray.FirstArraySlice = 0;
                uavDesc.Texture2DArray.ArraySize = 6;

                m_MipUAVDescriptors[mip] = resourceHeap->Allocate(RWTextureCube);
            }
            else
            {
                uavDesc.Texture2D.MipSlice = mip;

                m_MipUAVDescriptors[mip] = resourceHeap->Allocate(RWTexture2D);
            }

            d3dDevice->CreateUnorderedAccessView(m_Resource.Get(), nullptr, &uavDesc, resourceHeap->GetCPUHandle(m_MipUAVDescriptors[mip]));
        }
    }

    // RTV
    if (m_Description.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
    {
        DXGI_FORMAT rtvFormat = m_Description.Format;

        if (m_Description.Format == DXGI_FORMAT_D24_UNORM_S8_UINT)
        {
            rtvFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        }
        else if (m_Description.Format == DXGI_FORMAT_D32_FLOAT)
        {
            rtvFormat = DXGI_FORMAT_R32_FLOAT;
        }

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = rtvFormat;
        rtvDesc.ViewDimension = m_Description.IsCubeMap ? D3D12_RTV_DIMENSION_TEXTURE2DARRAY : D3D12_RTV_DIMENSION_TEXTURE2D;

        m_MipRTVDescriptors.resize(m_Description.MipLevels);

        for (uint32_t mip = 0; mip < m_Description.MipLevels; mip++)
        {
            if (m_Description.IsCubeMap)
            {
                rtvDesc.Texture2DArray.MipSlice = mip;
                rtvDesc.Texture2DArray.FirstArraySlice = 0;
                rtvDesc.Texture2DArray.ArraySize = 6;

                m_MipRTVDescriptors[mip] = rtvHeap->Allocate();
            }
            else
            {
                rtvDesc.Texture2D.MipSlice = mip;

                m_MipRTVDescriptors[mip] = rtvHeap->Allocate();
            }

            d3dDevice->CreateRenderTargetView(m_Resource.Get(), &rtvDesc, rtvHeap->GetCPUHandle(m_MipRTVDescriptors[mip]));
        }
    }

    // DSV
    if (m_Description.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
    {
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = m_Description.Format;
        dsvDesc.ViewDimension = m_Description.IsCubeMap ? D3D12_DSV_DIMENSION_TEXTURE2DARRAY : D3D12_DSV_DIMENSION_TEXTURE2D;

        m_MipDSVDescriptors.resize(m_Description.MipLevels);

        for (uint32_t mip = 0; mip < m_Description.MipLevels; mip++)
        {
            if (m_Description.IsCubeMap)
            {
                dsvDesc.Texture2DArray.MipSlice = mip;
                dsvDesc.Texture2DArray.FirstArraySlice = 0;
                dsvDesc.Texture2DArray.ArraySize = 6;

                m_MipDSVDescriptors[mip] = dsvHeap->Allocate();
            }
            else
            {
                dsvDesc.Texture2D.MipSlice = mip;

                m_MipDSVDescriptors[mip] = dsvHeap->Allocate();
            }

            d3dDevice->CreateDepthStencilView(m_Resource.Get(), &dsvDesc, dsvHeap->GetCPUHandle(m_MipDSVDescriptors[mip]));
        }
    }
}

// ------------------------------------------------------------------------------------------------------------------------------------
uint32_t Texture::CalculateMaxMipCount(uint32_t width, uint32_t height)
{
    return (uint32_t)log2(std::max(width, height)) + 1;
}
