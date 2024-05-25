#pragma once

#include "core/core.h"
#include "directx12.h"

struct BufferDescription
{
    uint32_t ElementSize = 0;
    uint32_t ElementCount = 0;
    D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE;
    D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON;
    D3D12_HEAP_TYPE HeapType = D3D12_HEAP_TYPE_DEFAULT;
};

class Buffer
{
public:
    Buffer(const BufferDescription& description, const wchar_t* debugName = L"Unnamed Buffer");
    ~Buffer();

    inline uint32_t GetSize() const { return m_Description.ElementCount * m_Description.ElementSize; }
    inline uint32_t GetElementSize() const { return m_Description.ElementSize; }
    inline uint32_t GetElementCount() const { return m_Description.ElementCount; }
    inline D3D12_RESOURCE_FLAGS GetFlags() const { return m_Description.Flags; }
    inline D3D12_RESOURCE_STATES GetInitialState() const { return m_Description.InitialState; }
    inline D3D12_HEAP_TYPE GetHeapType() const { return m_Description.HeapType; }
    inline void* GetMappedData() const { return m_MappedData; }
    inline const ComPtr<ID3D12Resource2>& GetResource() const { return m_Resource; }
private:
    BufferDescription m_Description;
    ComPtr<ID3D12Resource2> m_Resource;
    void* m_MappedData;
};