#include "buffer.h"

#include "rendering/graphicscontext.h"

// ------------------------------------------------------------------------------------------------------------------------------------
Buffer::Buffer(const BufferDescription& description, const wchar_t* debugName)
    : m_Description(description), m_MappedData(nullptr)
{
    auto d3dDevice = GraphicsContext::GetInstance()->GetDevice();

    CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(GetSize(), m_Description.Flags);
    CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(m_Description.HeapType);
    D3D12_RESOURCE_STATES initialState = m_Description.InitialState;

    if (m_Description.HeapType == D3D12_HEAP_TYPE_UPLOAD)
    {
        initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    }
    else if (m_Description.HeapType == D3D12_HEAP_TYPE_READBACK)
    {
        initialState = D3D12_RESOURCE_STATE_COPY_DEST;
    }

    DXCall(d3dDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, initialState, nullptr, IID_PPV_ARGS(&m_Resource)));
    DXCall(m_Resource->SetName(debugName));

    if (m_Description.HeapType != D3D12_HEAP_TYPE_READBACK)
    {
        CreateViews();
    }

    if (m_Description.HeapType == D3D12_HEAP_TYPE_UPLOAD)
    {
        // CPU write-only
        D3D12_RANGE readRange = { 0, 0 };
        DXCall(m_Resource->Map(0, &readRange, &m_MappedData));
    }
    else if (m_Description.HeapType == D3D12_HEAP_TYPE_READBACK)
    {
        // CPU read-only
        DXCall(m_Resource->Map(0, nullptr, &m_MappedData));
    }
}

// ------------------------------------------------------------------------------------------------------------------------------------
Buffer::~Buffer()
{
    if (m_Description.HeapType == D3D12_HEAP_TYPE_UPLOAD)
    {
        m_Resource->Unmap(0, nullptr);
    }
    else if (m_Description.HeapType == D3D12_HEAP_TYPE_READBACK)
    {
        D3D12_RANGE writtenRange = { 0, 0 };
        m_Resource->Unmap(0, &writtenRange);
    }

    if (m_Description.HeapType != D3D12_HEAP_TYPE_READBACK)
    {
        if ((m_Description.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
        {
            GraphicsContext::GetInstance()->GetResourceDescriptorHeap()->ReleaseDescriptor(m_SRVDescriptor, true);
        }

        if (m_Description.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
        {
            GraphicsContext::GetInstance()->GetResourceDescriptorHeap()->ReleaseDescriptor(m_UAVDescriptor, true);
        }
    }

    GraphicsContext::GetInstance()->ReleaseResource(m_Resource.Detach());
}

// ------------------------------------------------------------------------------------------------------------------------------------
DescriptorIndex Buffer::GetSRV() const
{
    if (m_Description.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)
        return InvalidDescriptorIndex;

    return m_SRVDescriptor;
}

// ------------------------------------------------------------------------------------------------------------------------------------
DescriptorIndex Buffer::GetUAV() const
{
    if ((m_Description.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0)
        return InvalidDescriptorIndex;

    return m_UAVDescriptor;
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Buffer::CreateViews()
{
    auto d3dDevice = GraphicsContext::GetInstance()->GetDevice();
    SegregatedDescriptorHeap* resourceHeap = GraphicsContext::GetInstance()->GetResourceDescriptorHeap();

    // SRV
    m_SRVDescriptor = InvalidDescriptorIndex;
    if ((m_Description.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = m_Description.ElementCount;
        srvDesc.Buffer.StructureByteStride = m_Description.ElementSize;

        m_SRVDescriptor = resourceHeap->Allocate(ROBuffer);
        d3dDevice->CreateShaderResourceView(m_Resource.Get(), &srvDesc, resourceHeap->GetCPUHandle(m_SRVDescriptor));
    }

    // UAV
    m_UAVDescriptor = InvalidDescriptorIndex;
    if (m_Description.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = m_Description.ElementCount;
        uavDesc.Buffer.StructureByteStride = m_Description.ElementSize;

        m_UAVDescriptor = resourceHeap->Allocate(RWBuffer);
        d3dDevice->CreateUnorderedAccessView(m_Resource.Get(), nullptr, &uavDesc, resourceHeap->GetCPUHandle(m_UAVDescriptor));
    }
}
