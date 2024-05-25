#include "descriptorheap.h"
#include "rendering/graphicscontext.h"

// ------------------------------------------------------------------------------------------------------------------------------------
DescriptorHeap::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t capacity, const wchar_t* debugName)
    : m_Type(type), m_Capacity(capacity), m_Size(0)
{

    HEXRAY_ASSERT(capacity);
    HEXRAY_ASSERT_MSG((m_Type != D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER && capacity <= D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2) ||
                      (m_Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER && capacity <= D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE), "Descriptor heap capacity is too big");

    // RTV and DSV heaps cannot be shader visible
    bool shaderVisible = m_Type != D3D12_DESCRIPTOR_HEAP_TYPE_RTV && m_Type != D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    auto d3dDevice = GraphicsContext::GetInstance()->GetDevice();

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = m_Type;
    heapDesc.NumDescriptors = capacity;
    heapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.NodeMask = 0;

    DXCall(d3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_Heap)));
    DXCall(m_Heap->SetName(debugName));

    m_DescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(m_Type);
    m_CPUStartHandle = m_Heap->GetCPUDescriptorHandleForHeapStart();
    m_GPUStartHandle = shaderVisible ? m_Heap->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };
}

// ------------------------------------------------------------------------------------------------------------------------------------
CPUDescriptorHeap::CPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t capacity, const wchar_t* debugName)
    : DescriptorHeap(type, capacity, debugName)
{
    HEXRAY_ASSERT(type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV || type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)

    for (uint32_t i = 0; i < capacity; i++)
        m_FreeSlots.push(i);
}

// ------------------------------------------------------------------------------------------------------------------------------------
D3D12_CPU_DESCRIPTOR_HANDLE CPUDescriptorHeap::Allocate()
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    HEXRAY_ASSERT_MSG(!m_FreeSlots.empty(), "Heap is full");

    uint32_t descriptorIndex = m_FreeSlots.front();
    m_FreeSlots.pop();
    m_Size++;

    return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_CPUStartHandle, descriptorIndex, m_DescriptorSize);
}

// ------------------------------------------------------------------------------------------------------------------------------------
void CPUDescriptorHeap::ReleaseDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, bool deferredRelease)
{
    if (descriptor.ptr != 0)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        HEXRAY_ASSERT_MSG((descriptor.ptr - m_CPUStartHandle.ptr) % m_DescriptorSize == 0, "Descriptor has different type than the heap!");
        HEXRAY_ASSERT_MSG(descriptor.ptr >= m_CPUStartHandle.ptr && descriptor.ptr < m_CPUStartHandle.ptr + m_DescriptorSize * m_Capacity, "Descriptor does not belong to this heap!");

        // Add the index to the free slots array
        uint32_t descriptorIndex = (descriptor.ptr - m_CPUStartHandle.ptr) / m_DescriptorSize;
        uint32_t currentFrameIndex = GraphicsContext::GetInstance()->GetBackBufferIndex();

        if (deferredRelease)
        {
            m_DeferredReleaseDescriptors[currentFrameIndex].push_back(descriptorIndex);
        }
        else
        {
            m_Size--;
            m_FreeSlots.push(descriptorIndex);
        }
    }
}

// ------------------------------------------------------------------------------------------------------------------------------------
void CPUDescriptorHeap::ProcessDeferredReleases(uint32_t frameIndex)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    for (uint32_t index : m_DeferredReleaseDescriptors[frameIndex])
    {
        m_Size--;
        m_FreeSlots.push(index);
    }

    m_DeferredReleaseDescriptors[frameIndex].clear();
}

// ------------------------------------------------------------------------------------------------------------------------------------
ResourceDescriptorHeap::ResourceDescriptorHeap(uint32_t texture2DTableSize, uint32_t textureCubeTableSize, const wchar_t* debugName)
    : DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, texture2DTableSize + textureCubeTableSize, debugName),
    m_Texture2DTableSize(texture2DTableSize), m_TextureCubeTableSize(textureCubeTableSize)
{
    for (uint32_t i = 0; i < texture2DTableSize; i++)
        m_FreeTexture2DSlots.push(i);

    for (uint32_t i = 0; i < textureCubeTableSize; i++)
        m_FreeTextureCubeSlots.push(i);
}

// ------------------------------------------------------------------------------------------------------------------------------------
D3D12_GPU_DESCRIPTOR_HANDLE ResourceDescriptorHeap::AllocateTexture2D()
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    HEXRAY_ASSERT_MSG(!m_FreeTexture2DSlots.empty(), "Texture2D descriptor table is full");

    uint32_t descriptorIndex = m_FreeTexture2DSlots.front();
    m_FreeTexture2DSlots.pop();
    m_Size++;

    return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_GPUStartHandle, descriptorIndex, m_DescriptorSize);
}

// ------------------------------------------------------------------------------------------------------------------------------------
D3D12_GPU_DESCRIPTOR_HANDLE ResourceDescriptorHeap::AllocateTextureCube()
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    HEXRAY_ASSERT_MSG(!m_FreeTextureCubeSlots.empty(), "TextureCube descriptor table is full");

    uint32_t descriptorIndex = m_FreeTextureCubeSlots.front();
    m_FreeTextureCubeSlots.pop();
    m_Size++;

    return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_GPUStartHandle, descriptorIndex + m_Texture2DTableSize, m_DescriptorSize);
}

// ------------------------------------------------------------------------------------------------------------------------------------
void ResourceDescriptorHeap::ReleaseDescriptor(D3D12_GPU_DESCRIPTOR_HANDLE descriptor, bool deferredRelease)
{
    if (descriptor.ptr != 0)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        HEXRAY_ASSERT_MSG((descriptor.ptr - m_GPUStartHandle.ptr) % m_DescriptorSize == 0, "Descriptor has different type than the heap!");
        HEXRAY_ASSERT_MSG(descriptor.ptr >= m_GPUStartHandle.ptr && descriptor.ptr < m_GPUStartHandle.ptr + m_DescriptorSize * m_Capacity, "Descriptor does not belong to this heap!");

        // Add the index to the free slots array
        uint32_t descriptorIndex = (descriptor.ptr - m_GPUStartHandle.ptr) / m_DescriptorSize;
        uint32_t currentFrameIndex = GraphicsContext::GetInstance()->GetBackBufferIndex();

        if (deferredRelease)
        {
            m_DeferredReleaseDescriptors[currentFrameIndex].push_back(descriptorIndex);
        }
        else
        {
            m_Size--;

            if(descriptorIndex < m_Texture2DTableSize)
                m_FreeTexture2DSlots.push(descriptorIndex);
            else
                m_FreeTextureCubeSlots.push(descriptorIndex);
        }
    }
}

// ------------------------------------------------------------------------------------------------------------------------------------
void ResourceDescriptorHeap::ProcessDeferredReleases(uint32_t frameIndex)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    for (uint32_t index : m_DeferredReleaseDescriptors[frameIndex])
    {
        m_Size--;

        if (index < m_Texture2DTableSize)
            m_FreeTexture2DSlots.push(index);
        else
            m_FreeTextureCubeSlots.push(index);
    }

    m_DeferredReleaseDescriptors[frameIndex].clear();
}

// ------------------------------------------------------------------------------------------------------------------------------------
SamplerDescriptorHeap::SamplerDescriptorHeap(uint32_t capacity, const wchar_t* debugName)
    : DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, capacity, debugName)
{
    for (uint32_t i = 0; i < capacity; i++)
        m_FreeSlots.push(i);
}

// ------------------------------------------------------------------------------------------------------------------------------------
D3D12_GPU_DESCRIPTOR_HANDLE SamplerDescriptorHeap::Allocate()
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    HEXRAY_ASSERT_MSG(!m_FreeSlots.empty(), "Heap is full");

    uint32_t descriptorIndex = m_FreeSlots.front();
    m_FreeSlots.pop();
    m_Size++;

    return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_GPUStartHandle, descriptorIndex, m_DescriptorSize);
}

// ------------------------------------------------------------------------------------------------------------------------------------
void SamplerDescriptorHeap::ReleaseDescriptor(D3D12_GPU_DESCRIPTOR_HANDLE descriptor, bool deferredRelease)
{
    if (descriptor.ptr != 0)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        HEXRAY_ASSERT_MSG((descriptor.ptr - m_GPUStartHandle.ptr) % m_DescriptorSize == 0, "Descriptor has different type than the heap!");
        HEXRAY_ASSERT_MSG(descriptor.ptr >= m_GPUStartHandle.ptr && descriptor.ptr < m_GPUStartHandle.ptr + m_DescriptorSize * m_Capacity, "Descriptor does not belong to this heap!");

        // Add the index to the free slots array
        uint32_t descriptorIndex = (descriptor.ptr - m_GPUStartHandle.ptr) / m_DescriptorSize;
        uint32_t currentFrameIndex = GraphicsContext::GetInstance()->GetBackBufferIndex();

        if (deferredRelease)
        {
            m_DeferredReleaseDescriptors[currentFrameIndex].push_back(descriptorIndex);
        }
        else
        {
            m_Size--;
            m_FreeSlots.push(descriptorIndex);
        }
    }
}

// ------------------------------------------------------------------------------------------------------------------------------------
void SamplerDescriptorHeap::ProcessDeferredReleases(uint32_t frameIndex)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    for (uint32_t index : m_DeferredReleaseDescriptors[frameIndex])
    {
        m_Size--;
        m_FreeSlots.push(index);
    }

    m_DeferredReleaseDescriptors[frameIndex].clear();
}