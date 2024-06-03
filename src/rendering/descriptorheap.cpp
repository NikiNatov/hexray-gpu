#include "descriptorheap.h"
#include "rendering/graphicscontext.h"

// ------------------------------------------------------------------------------------------------------------------------------------
DescriptorHeapBase::DescriptorHeapBase(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t capacity, const wchar_t* debugName)
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
StandardDescriptorHeap::StandardDescriptorHeap(const StandardDescriptorHeapDescription& description, const wchar_t* debugName)
    : DescriptorHeapBase(description.Type, description.Capacity, debugName)
{
    for (uint32_t i = 0; i < description.Capacity; i++)
        m_FreeSlots.push(i);
}

// ------------------------------------------------------------------------------------------------------------------------------------
DescriptorIndex StandardDescriptorHeap::Allocate()
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    HEXRAY_ASSERT_MSG(!m_FreeSlots.empty(), "Heap is full");

    uint32_t descriptorIndex = m_FreeSlots.front();
    m_FreeSlots.pop();
    m_Size++;

    return descriptorIndex;
}

// ------------------------------------------------------------------------------------------------------------------------------------
void StandardDescriptorHeap::ReleaseDescriptor(DescriptorIndex descriptorIndex, bool deferredRelease)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

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

// ------------------------------------------------------------------------------------------------------------------------------------
void StandardDescriptorHeap::ProcessDeferredReleases(uint32_t frameIndex)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    for (DescriptorIndex index : m_DeferredReleaseDescriptors[frameIndex])
    {
        m_Size--;
        m_FreeSlots.push(index);
    }

    m_DeferredReleaseDescriptors[frameIndex].clear();
}

// ------------------------------------------------------------------------------------------------------------------------------------
SegregatedDescriptorHeap::SegregatedDescriptorHeap(const SegregatedDescriptorHeapDescription& description, const wchar_t* debugName)
    : DescriptorHeapBase(
        description.Type,
        description.DescriptorTableSizes[ROTexture2D] + description.DescriptorTableSizes[RWTexture2D] +
        description.DescriptorTableSizes[ROTextureCube] + description.DescriptorTableSizes[RWTextureCube] +
        description.DescriptorTableSizes[ROBuffer] + description.DescriptorTableSizes[RWBuffer] +
        description.DescriptorTableSizes[AccelerationStructure],
        debugName
    ), m_Description(description)
{
    HEXRAY_ASSERT_MSG(description.Type != D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, "Segrageted heaps do not currently support sampler type");

    uint32_t currentOffset = 0;
    for (uint32_t type = 0; type < NumDescriptorTypes; type++)
    {
        m_DescriptorTableOffsets[type] = currentOffset;

        for (uint32_t j = 0; j < description.DescriptorTableSizes[type]; j++)
        {
            m_FreeSlots[type].push(j);
        }

        currentOffset += description.DescriptorTableSizes[type];
    }
}

// ------------------------------------------------------------------------------------------------------------------------------------
DescriptorIndex SegregatedDescriptorHeap::Allocate(DescriptorType descriptorType)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    HEXRAY_ASSERT_MSG(!m_FreeSlots[descriptorType].empty(), "Descriptor table for type {} is full", descriptorType);

    uint32_t descriptorIndex = m_FreeSlots[descriptorType].front();
    m_FreeSlots[descriptorType].pop();
    m_Size++;

    return descriptorIndex;
}

// ------------------------------------------------------------------------------------------------------------------------------------
void SegregatedDescriptorHeap::ReleaseDescriptor(DescriptorIndex descriptorIndex, DescriptorType descriptorType, bool deferredRelease)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    uint32_t currentFrameIndex = GraphicsContext::GetInstance()->GetBackBufferIndex();
    if (deferredRelease)
    {
        m_DeferredReleaseDescriptors[currentFrameIndex].push_back({ descriptorIndex, descriptorType });
    }
    else
    {
        m_Size--;
        m_FreeSlots[descriptorType].push(descriptorIndex);
    }
}

// ------------------------------------------------------------------------------------------------------------------------------------
void SegregatedDescriptorHeap::ProcessDeferredReleases(uint32_t frameIndex)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    for (auto& [descriptorIndex, descriptorType] : m_DeferredReleaseDescriptors[frameIndex])
    {
        m_Size--;
        m_FreeSlots[descriptorType].push(descriptorIndex);
    }

    m_DeferredReleaseDescriptors[frameIndex].clear();
}

// ------------------------------------------------------------------------------------------------------------------------------------
uint32_t SegregatedDescriptorHeap::GetShaderSpaceForDescriptorType(DescriptorType type)
{
    switch (type)
    {
        case DescriptorType::ROTexture2D: return 0;
        case DescriptorType::ROTextureCube: return 1;
        case DescriptorType::ROBuffer: return 2;
        case DescriptorType::AccelerationStructure: return 3;
        case DescriptorType::RWTexture2D: return 0;
        case DescriptorType::RWTextureCube: return 1;
        case DescriptorType::RWBuffer: return 2;
    }

    HEXRAY_ASSERT_MSG(false, "Unknown descriptor type");
    return -1;
}

// ------------------------------------------------------------------------------------------------------------------------------------
bool SegregatedDescriptorHeap::IsDescriptorTypeReadOnly(DescriptorType type)
{
    switch (type)
    {
        case DescriptorType::ROTexture2D:
        case DescriptorType::ROTextureCube:
        case DescriptorType::ROBuffer:
        case DescriptorType::AccelerationStructure:
            return true;
        case DescriptorType::RWTexture2D:
        case DescriptorType::RWTextureCube:
        case DescriptorType::RWBuffer:
            return false;
    }

    HEXRAY_ASSERT_MSG(false, "Unknown descriptor type");
    return false;
}
