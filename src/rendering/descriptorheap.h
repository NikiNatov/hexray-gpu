#pragma once

#include "core/core.h"
#include "rendering/directx12.h"

#include <queue>

#define TEXTURE2D_BINDLESS_TABLE_SIZE 4096
#define TEXTURECUBE_BINDLESS_TABLE_SIZE 1024

class DescriptorHeap
{
public:
    virtual ~DescriptorHeap() = default;

    inline D3D12_DESCRIPTOR_HEAP_TYPE GetType() const { return m_Type; }
    inline uint32_t GetDescriptorSize() const { return m_DescriptorSize; }
    inline uint32_t GetCapacity() const { return m_Capacity; }
    inline D3D12_CPU_DESCRIPTOR_HANDLE GetCPUStartHandle() const { return m_CPUStartHandle; }
    inline D3D12_GPU_DESCRIPTOR_HANDLE GetGPUStartHandle() const { return m_GPUStartHandle; }
    inline const ComPtr<ID3D12DescriptorHeap>& GetHeap() const { return m_Heap; }
protected:
    DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t capacity, const wchar_t* debugName = L"Unnamed Descriptor Heap");
protected:
    D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
    D3D12_CPU_DESCRIPTOR_HANDLE m_CPUStartHandle{ 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE m_GPUStartHandle{ 0 };
    uint32_t m_Capacity;
    uint32_t m_Size;
    uint32_t m_DescriptorSize;
    ComPtr<ID3D12DescriptorHeap> m_Heap;
};

class CPUDescriptorHeap : public DescriptorHeap
{
public:
    CPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t capacity, const wchar_t* debugName = L"Unnamed CPU Descriptor Heap");

    D3D12_CPU_DESCRIPTOR_HANDLE Allocate();
    void ReleaseDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, bool deferredRelease);
    void ProcessDeferredReleases(uint32_t frameIndex);
private:
    std::queue<uint32_t> m_FreeSlots;
    std::vector<uint32_t> m_DeferredReleaseDescriptors[FRAMES_IN_FLIGHT];
    std::mutex m_Mutex;
};

class ResourceDescriptorHeap : public DescriptorHeap
{
public:
    ResourceDescriptorHeap(uint32_t texture2DTableSize, uint32_t textureCubeTableSize, const wchar_t* debugName = L"Unnamed Resource Descriptor Heap");

    D3D12_GPU_DESCRIPTOR_HANDLE AllocateTexture2D();
    D3D12_GPU_DESCRIPTOR_HANDLE AllocateTextureCube();

    void ReleaseDescriptor(D3D12_GPU_DESCRIPTOR_HANDLE descriptor, bool deferredRelease);
    void ProcessDeferredReleases(uint32_t frameIndex);
private:
    uint32_t m_Texture2DTableSize;
    uint32_t m_TextureCubeTableSize;
    std::queue<uint32_t> m_FreeTexture2DSlots;
    std::queue<uint32_t> m_FreeTextureCubeSlots;
    std::vector<uint32_t> m_DeferredReleaseDescriptors[FRAMES_IN_FLIGHT];
    std::mutex m_Mutex;
};

class SamplerDescriptorHeap : public DescriptorHeap
{
public:
    SamplerDescriptorHeap(uint32_t capacity, const wchar_t* debugName = L"Unnamed Sampler Descriptor Heap");

    D3D12_GPU_DESCRIPTOR_HANDLE Allocate();
    void ReleaseDescriptor(D3D12_GPU_DESCRIPTOR_HANDLE descriptor, bool deferredRelease);
    void ProcessDeferredReleases(uint32_t frameIndex);
private:
    std::queue<uint32_t> m_FreeSlots;
    std::vector<uint32_t> m_DeferredReleaseDescriptors[FRAMES_IN_FLIGHT];
    std::mutex m_Mutex;
};