#pragma once

#include "core/core.h"
#include "rendering/directx12.h"

#include <queue>

using DescriptorIndex = uint32_t;
static constexpr DescriptorIndex InvalidDescriptorIndex = UINT32_MAX;

class DescriptorHeapBase
{
public:
    virtual ~DescriptorHeapBase() = default;

    inline D3D12_CPU_DESCRIPTOR_HANDLE GetBaseCPUHandle() const { return m_CPUStartHandle; }
    inline D3D12_GPU_DESCRIPTOR_HANDLE GetBaseGPUHandle() const { return m_GPUStartHandle; }
    inline D3D12_DESCRIPTOR_HEAP_TYPE GetType() const { return m_Type; }
    inline uint32_t GetDescriptorSize() const { return m_DescriptorSize; }
    inline uint32_t GetCapacity() const { return m_Capacity; }
    inline const ComPtr<ID3D12DescriptorHeap>& GetHeap() const { return m_Heap; }
protected:
    DescriptorHeapBase(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t capacity, const wchar_t* debugName = L"Unnamed Descriptor Heap");
protected:
    D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
    D3D12_CPU_DESCRIPTOR_HANDLE m_CPUStartHandle{ 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE m_GPUStartHandle{ 0 };
    uint32_t m_Capacity;
    uint32_t m_Size;
    uint32_t m_DescriptorSize;
    ComPtr<ID3D12DescriptorHeap> m_Heap;
};

struct StandardDescriptorHeapDescription
{
    D3D12_DESCRIPTOR_HEAP_TYPE Type;
    uint32_t Capacity;
};

class StandardDescriptorHeap : public DescriptorHeapBase
{
public:
    StandardDescriptorHeap(const StandardDescriptorHeapDescription& description, const wchar_t* debugName = L"Unnamed Standard Descriptor Heap");

    DescriptorIndex Allocate();

    void ReleaseDescriptor(DescriptorIndex descriptorIndex, bool deferredRelease);
    void ProcessDeferredReleases(uint32_t frameIndex);

    inline D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(DescriptorIndex index) const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_CPUStartHandle, index, m_DescriptorSize); }
    inline D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(DescriptorIndex index) const { return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_GPUStartHandle, index, m_DescriptorSize); }
private:
    std::queue<DescriptorIndex> m_FreeSlots;
    std::vector<DescriptorIndex> m_DeferredReleaseDescriptors[FRAMES_IN_FLIGHT];
    std::mutex m_Mutex;
};

enum DescriptorType
{
    ROTexture2D,
    RWTexture2D,
    ROTextureCube,
    RWTextureCube,
    ROBuffer,
    RWBuffer,
    AccelerationStructure,
    NumDescriptorTypes
};

struct SegregatedDescriptorHeapDescription
{
    D3D12_DESCRIPTOR_HEAP_TYPE Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    uint32_t DescriptorTableSizes[NumDescriptorTypes];
};

class SegregatedDescriptorHeap : public DescriptorHeapBase
{
public:
    SegregatedDescriptorHeap(const SegregatedDescriptorHeapDescription& description, const wchar_t* debugName = L"Unnamed Segregated Descriptor Heap");

    DescriptorIndex Allocate(DescriptorType descriptorType);

    void ReleaseDescriptor(DescriptorIndex descriptor, DescriptorType descriptorType, bool deferredRelease);
    void ProcessDeferredReleases(uint32_t frameIndex);

    inline const SegregatedDescriptorHeapDescription& GetDescription() const { return m_Description; }
    inline D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(DescriptorIndex index, DescriptorType type) const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_CPUStartHandle, index + m_DescriptorTableOffsets[type], m_DescriptorSize); }
    inline D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(DescriptorIndex index, DescriptorType type) const { return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_GPUStartHandle, index + m_DescriptorTableOffsets[type], m_DescriptorSize); }
public:
    static uint32_t GetShaderSpaceForDescriptorType(DescriptorType type);
    static bool IsDescriptorTypeReadOnly(DescriptorType type);
private:
    SegregatedDescriptorHeapDescription m_Description;
    uint32_t m_DescriptorTableOffsets[NumDescriptorTypes];
    std::queue<DescriptorIndex> m_FreeSlots[NumDescriptorTypes];
    std::vector<std::pair<DescriptorIndex, DescriptorType>> m_DeferredReleaseDescriptors[FRAMES_IN_FLIGHT];
    std::mutex m_Mutex;
};