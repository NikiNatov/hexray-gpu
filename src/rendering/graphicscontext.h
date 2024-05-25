#pragma once

#include "core/core.h"
#include "core/window.h"
#include "rendering/directx12.h"
#include "rendering/descriptorheap.h"

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

    inline bool IsDebugLayerEnabled() const { return m_Description.EnableDebugLayer; }
    inline bool IsHardwareRayTracingSupported() const { return m_HardwareRayTracingSupported; }
    inline bool IsTearingSupported() const { return m_TearingSupported; }

    inline const DXGI_ADAPTER_DESC3& GetAdapterDescription() const { return m_AdapterDescription; }
    inline const ComPtr<IDXGIFactory7>& GetFactory() const { return m_Factory; }
    inline const ComPtr<IDXGIAdapter4>& GetAdapter() const { return m_Adapter; }
    inline const ComPtr<ID3D12Device6>& GetDevice() const { return m_Device; }

    inline const ComPtr<ID3D12CommandQueue>& GetGraphicsQueue() const { return m_GraphicsQueue; }
    inline const ComPtr<ID3D12CommandList>& GetCommandList() const { return m_CommandList; }

    inline CPUDescriptorHeap* GetRTVDescriptorHeap() const { return m_RTVDescriptorHeap.get(); }
    inline CPUDescriptorHeap* GetDSVDescriptorHeap() const { return m_DSVDescriptorHeap.get(); }
    inline ResourceDescriptorHeap* GetResourceDescriptorHeap() const { return m_ResourceDescriptorHeap.get(); }
    inline SamplerDescriptorHeap* GetSamplerDescriptorHeap() const { return m_SamplerDescriptorHeap.get(); }

    inline const ComPtr<IDXGISwapChain4>& GetSwapChain() const { return m_SwapChain; }
    inline uint64_t GetBackBufferIndex() const { return m_BackBufferIndex; }
public:
    static GraphicsContext* GetInstance() { return ms_Instance; }
private:
    void RecreateRenderTargetViews(uint32_t width, uint32_t height);
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
    ComPtr<ID3D12CommandAllocator> m_CommandAllocators[FRAMES_IN_FLIGHT];
    ComPtr<ID3D12GraphicsCommandList> m_CommandList;

    // Synchronization objects
    ComPtr<ID3D12Fence1> m_FrameFence;
    uint64_t m_FenceValue;
    uint64_t m_FrameFenceValues[FRAMES_IN_FLIGHT];
    HANDLE m_FrameFenceEvent;

    // Descriptor heap objects
    std::unique_ptr<CPUDescriptorHeap> m_RTVDescriptorHeap;
    std::unique_ptr<CPUDescriptorHeap> m_DSVDescriptorHeap;
    std::unique_ptr<ResourceDescriptorHeap> m_ResourceDescriptorHeap;
    std::unique_ptr<SamplerDescriptorHeap> m_SamplerDescriptorHeap;

    // Swap chain objects
    ComPtr<IDXGISwapChain4> m_SwapChain;
    ComPtr<ID3D12Resource2> m_BackBuffers[FRAMES_IN_FLIGHT];
    D3D12_CPU_DESCRIPTOR_HANDLE m_BackBufferRTVs[FRAMES_IN_FLIGHT];
    uint32_t m_SwapChainWidth;
    uint32_t m_SwapChainHeight;
    uint64_t m_BackBufferIndex;
    D3D12_VIEWPORT m_Viewport;
    D3D12_RECT m_ScissorRect;

    // Debug layer objects
    ComPtr<ID3D12Debug5> m_DebugLayer;
    ComPtr<IDXGIDebug1> m_DXGILayer;
    ComPtr<ID3D12InfoQueue> m_InfoQueue;
private:
    static GraphicsContext* ms_Instance;
};