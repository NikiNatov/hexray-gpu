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

    GraphicsContext::GetInstance()->ReleaseResource(m_Resource.Detach());
}
