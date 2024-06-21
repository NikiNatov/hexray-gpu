#pragma once

#include "core/core.h"
#include "core/window.h"
#include "rendering/directx12.h"
#include "rendering/descriptorheap.h"
#include "rendering/buffer.h"
#include "rendering/mesh.h"
#include "rendering/renderer.h"

struct GraphicsContextDescription
{
    Window* Window = nullptr;
    bool EnableDebugLayer = true;
};

class GraphicsContext
{
public:
    GraphicsContext(const GraphicsContextDescription& description);
    ~GraphicsContext();

    void BeginFrame();
    void EndFrame();
    void ResizeSwapChain(uint32_t width, uint32_t height);
    void WaitForGPU();
    void ProcessDeferredReleases(uint64_t frameIndex);
    void ReleaseResource(ID3D12Resource2* resource, bool deferredRelease = true);
    void UploadBufferData(Buffer* destBuffer, const void* data);
    void UploadTextureData(Texture* destTexture, const void* data, uint32_t mip = 0, uint32_t face = 0);
    void DispatchRays(uint32_t width, uint32_t height, const ResourceBindTable& resourceBindings, const RaytracingPipeline* pipeline);
    void DispatchComputeShader(uint32_t threadCountX, uint32_t threadCountY, uint32_t threadCountZ, const ResourceBindTable& resourceBindings, const ComputePipeline* pipeline);
    void CopyTextureToSwapChain(Texture* texture, D3D12_RESOURCE_STATES textureCurrentState);
    void AddUAVBarrier(Texture* texture);
    std::shared_ptr<Buffer> BuildBottomLevelAccelerationStructure(Mesh* mesh, uint32_t submeshIndex);
    std::shared_ptr<Buffer> BuildTopLevelAccelerationStructure(const std::vector<MeshInstance>& meshInstances);

    inline bool IsDebugLayerEnabled() const { return m_Description.EnableDebugLayer; }
    inline bool IsHardwareRayTracingSupported() const { return m_HardwareRayTracingSupported; }
    inline bool IsTearingSupported() const { return m_TearingSupported; }

    inline const DXGI_ADAPTER_DESC3& GetAdapterDescription() const { return m_AdapterDescription; }
    inline const ComPtr<IDXGIFactory7>& GetFactory() const { return m_Factory; }
    inline const ComPtr<IDXGIAdapter4>& GetAdapter() const { return m_Adapter; }
    inline const ComPtr<ID3D12Device6>& GetDevice() const { return m_Device; }

    inline const ComPtr<ID3D12CommandQueue>& GetGraphicsQueue() const { return m_GraphicsQueue; }
    inline const ComPtr<ID3D12GraphicsCommandList6>& GetCommandList() const { return m_CommandList; }

    inline StandardDescriptorHeap* GetRTVDescriptorHeap() const { return m_RTVDescriptorHeap.get(); }
    inline StandardDescriptorHeap* GetDSVDescriptorHeap() const { return m_DSVDescriptorHeap.get(); }
    inline SegregatedDescriptorHeap* GetResourceDescriptorHeap() const { return m_ResourceDescriptorHeap.get(); }

    inline const ComPtr<IDXGISwapChain4>& GetSwapChain() const { return m_SwapChain; }
    inline uint64_t GetBackBufferIndex() const { return m_BackBufferIndex; }

    inline const ComPtr<ID3D12RootSignature>& GetBindlessRootSignature() const { return m_BindlessRootSignature; }
public:
    static GraphicsContext* GetInstance() { return ms_Instance; }
private:
    void RecreateRenderTargetViews(uint32_t width, uint32_t height);

    void CreateDevice();
    void CreateCommandQueues();
    void CreateDescriptorHeaps();
    void CreateSwapChainAndSyncPrimitives();
    void CreateRootSignatures();
private:
    GraphicsContextDescription m_Description;
    bool m_HardwareRayTracingSupported;
    bool m_TearingSupported;

    // Device objects
    DXGI_ADAPTER_DESC3 m_AdapterDescription;
    ComPtr<IDXGIFactory7> m_Factory;
    ComPtr<IDXGIAdapter4> m_Adapter;
    ComPtr<ID3D12Device6> m_Device;

    // Command queue objects
    ComPtr<ID3D12CommandQueue> m_GraphicsQueue;
    ComPtr<ID3D12CommandQueue> m_CopyQueue;
    ComPtr<ID3D12CommandAllocator> m_CommandAllocators[FRAMES_IN_FLIGHT];
    ComPtr<ID3D12GraphicsCommandList6> m_CommandList;

    // Synchronization objects
    ComPtr<ID3D12Fence1> m_FrameFence;
    uint64_t m_FenceValue;
    uint64_t m_FrameFenceValues[FRAMES_IN_FLIGHT];
    HANDLE m_FrameFenceEvent;

    // Descriptor heap objects
    std::unique_ptr<StandardDescriptorHeap> m_RTVDescriptorHeap;
    std::unique_ptr<StandardDescriptorHeap> m_DSVDescriptorHeap;
    std::unique_ptr<SegregatedDescriptorHeap> m_ResourceDescriptorHeap;

    // Swap chain objects
    ComPtr<IDXGISwapChain4> m_SwapChain;
    ComPtr<ID3D12Resource2> m_BackBuffers[FRAMES_IN_FLIGHT];
    DescriptorIndex m_BackBufferRTVs[FRAMES_IN_FLIGHT];
    uint32_t m_SwapChainWidth;
    uint32_t m_SwapChainHeight;
    uint64_t m_BackBufferIndex;
    D3D12_VIEWPORT m_Viewport;
    D3D12_RECT m_ScissorRect;

    // Debug layer objects
    ComPtr<ID3D12Debug5> m_DebugLayer;
    ComPtr<IDXGIDebug1> m_DXGILayer;
    ComPtr<ID3D12InfoQueue> m_InfoQueue;

    // Deferred release resources
    std::vector<ID3D12Resource2*> m_DeferredReleaseResources[FRAMES_IN_FLIGHT];
    std::mutex m_DeferredReleaseMutex;

    // Root signatures:
    // Since we use bindless resources, we only need one root signature that is shared across all pipelines
    ComPtr<ID3D12RootSignature> m_BindlessRootSignature;
private:
    static GraphicsContext* ms_Instance;
};