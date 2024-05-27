#include "graphicscontext.h"

GraphicsContext* GraphicsContext::ms_Instance = nullptr;

// ------------------------------------------------------------------------------------------------------------------------------------
GraphicsContext::GraphicsContext(const GraphicsContextDescription& description)
    : m_Description(description)
{
    HEXRAY_ASSERT(!ms_Instance, "GraphicsContext already created");
    ms_Instance = this;

    CreateDevice();
    CreateCommandQueues();
    CreateDescriptorHeaps();
    CreateSwapChainAndSyncPrimitives();
    CreateRootSignatures();
}

// ------------------------------------------------------------------------------------------------------------------------------------
GraphicsContext::~GraphicsContext()
{
    WaitForGPU();

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        m_CommandAllocators[i].Reset();
        m_BackBuffers[i].Reset();
        m_RTVDescriptorHeap->ReleaseDescriptor(m_BackBufferRTVs[i], true);

        ProcessDeferredReleases(i);
    }

    m_BindlessRootSignature.Reset();
    m_SwapChain.Reset();

    CloseHandle(m_FrameFenceEvent);
    m_FrameFence.Reset();

    m_RTVDescriptorHeap.reset();
    m_DSVDescriptorHeap.reset();
    m_ResourceDescriptorHeap.reset();

    m_CommandList.Reset();
    m_GraphicsQueue.Reset();

    m_Device.Reset();
    m_Adapter.Reset();
    m_Factory.Reset();

    if (m_Description.EnableDebugLayer)
    {
        m_InfoQueue.Reset();
        DXCall(m_DXGILayer->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL)));
        m_DXGILayer.Reset();
        m_DebugLayer.Reset();
    }
}

// ------------------------------------------------------------------------------------------------------------------------------------
void GraphicsContext::BeginFrame()
{
    m_BackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is ready.
    if (m_FrameFence->GetCompletedValue() < m_FrameFenceValues[m_BackBufferIndex])
    {
        DXCall(m_FrameFence->SetEventOnCompletion(m_FrameFenceValues[m_BackBufferIndex], m_FrameFenceEvent));
        WaitForSingleObjectEx(m_FrameFenceEvent, INFINITE, FALSE);
    }

    ProcessDeferredReleases(m_BackBufferIndex);

    DXCall(m_CommandAllocators[m_BackBufferIndex]->Reset());
    DXCall(m_CommandList->Reset(m_CommandAllocators[m_BackBufferIndex].Get(), nullptr));

    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_BackBuffers[m_BackBufferIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);;
    m_CommandList->ResourceBarrier(1, &barrier);

    const float clearColor[] = { 0.8f, 0.3f, 0.1f, 1.0f };
    m_CommandList->ClearRenderTargetView(m_RTVDescriptorHeap->GetCPUHandle(m_BackBufferRTVs[m_BackBufferIndex]), clearColor, 1, &m_ScissorRect);
}

// ------------------------------------------------------------------------------------------------------------------------------------
void GraphicsContext::EndFrame()
{
    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_BackBuffers[m_BackBufferIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);;
    m_CommandList->ResourceBarrier(1, &barrier);

    DXCall(m_CommandList->Close());

    ID3D12CommandList* commandLists[] = { m_CommandList.Get() };
    m_GraphicsQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    bool vsync = m_Description.Window->IsVSyncOn();
    DXCall(m_SwapChain->Present(vsync, !vsync && m_TearingSupported ? DXGI_PRESENT_ALLOW_TEARING : 0));

    m_FrameFenceValues[m_BackBufferIndex] = ++m_FenceValue;
    DXCall(m_GraphicsQueue->Signal(m_FrameFence.Get(), m_FrameFenceValues[m_BackBufferIndex]));
}

// ------------------------------------------------------------------------------------------------------------------------------------
void GraphicsContext::ResizeSwapChain(uint32_t width, uint32_t height)
{
    if (m_SwapChainWidth != width || m_SwapChainHeight != height)
    {
        WaitForGPU();

        for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
            m_BackBuffers[i].Reset();

        DXCall(m_SwapChain->ResizeBuffers(FRAMES_IN_FLIGHT, 0, 0, DXGI_FORMAT_UNKNOWN, m_TearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0));
        m_BackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

        RecreateRenderTargetViews(width, height);
    }
}

// ------------------------------------------------------------------------------------------------------------------------------------
void GraphicsContext::WaitForGPU()
{
    HANDLE waitFenceEvent = CreateEvent(0, false, false, 0);
    ComPtr<ID3D12Fence1> waitFence;
    DXCall(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&waitFence)));
    DXCall(waitFence->SetName(L"GPU Wait Fence"));

    DXCall(m_GraphicsQueue->Signal(waitFence.Get(), 1));
    DXCall(waitFence->SetEventOnCompletion(1, waitFenceEvent));
    WaitForSingleObjectEx(waitFenceEvent, INFINITE, FALSE);

    CloseHandle(waitFenceEvent);
}

// ------------------------------------------------------------------------------------------------------------------------------------
void GraphicsContext::ProcessDeferredReleases(uint64_t frameIndex)
{
    m_RTVDescriptorHeap->ProcessDeferredReleases(frameIndex);
    m_DSVDescriptorHeap->ProcessDeferredReleases(frameIndex);
    m_ResourceDescriptorHeap->ProcessDeferredReleases(frameIndex);

    std::lock_guard<std::mutex> lock(m_DeferredReleaseMutex);

    for (ID3D12Resource2* resource : m_DeferredReleaseResources[frameIndex])
        resource->Release();

    m_DeferredReleaseResources[frameIndex].clear();
}

// ------------------------------------------------------------------------------------------------------------------------------------
void GraphicsContext::ReleaseResource(ID3D12Resource2* resource, bool deferredRelease)
{
    if (!resource)
        return;

    if (!deferredRelease)
    {
        resource->Release();
        return;
    }

    std::lock_guard<std::mutex> lock(m_DeferredReleaseMutex);
    m_DeferredReleaseResources[m_BackBufferIndex].push_back(resource);
}

// ------------------------------------------------------------------------------------------------------------------------------------
void GraphicsContext::RecreateRenderTargetViews(uint32_t width, uint32_t height)
{
    // Recreate back buffer textures
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        DXCall(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_BackBuffers[i])));
        DXCall(m_BackBuffers[i]->SetName(fmt::format(L"Back Buffer {}", i).c_str()));

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        m_Device->CreateRenderTargetView(m_BackBuffers[i].Get(), &rtvDesc, m_RTVDescriptorHeap->GetCPUHandle(m_BackBufferRTVs[i]));
    }

    // Recreate viewport
    m_Viewport.TopLeftX = 0;
    m_Viewport.TopLeftY = 0;
    m_Viewport.Width = width;
    m_Viewport.Height = height;
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;

    // Recreate scissor rect
    m_ScissorRect = { 0, 0, (int)width, (int)height };
}

// ------------------------------------------------------------------------------------------------------------------------------------
void GraphicsContext::CreateDevice()
{
    // Create factory
    uint32_t factoryCreateFlags = m_Description.EnableDebugLayer ? DXGI_CREATE_FACTORY_DEBUG : 0;
    DXCall(CreateDXGIFactory2(factoryCreateFlags, IID_PPV_ARGS(&m_Factory)));

    // Query all adapters and pick the one with the most video memory
    uint64_t currentMaxVideoMemory = 0;
    ComPtr<IDXGIAdapter> dxgiAdapter = nullptr;

    for (uint32_t i = 0; m_Factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&dxgiAdapter)) != DXGI_ERROR_NOT_FOUND; i++)
    {
        DXGI_ADAPTER_DESC desc;
        dxgiAdapter->GetDesc(&desc);

        if (SUCCEEDED(D3D12CreateDevice(dxgiAdapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)) && desc.DedicatedVideoMemory > currentMaxVideoMemory)
        {
            currentMaxVideoMemory = desc.DedicatedVideoMemory;
            DXCall(dxgiAdapter.As(&m_Adapter));
        }
    }

    m_Adapter->GetDesc3(&m_AdapterDescription);

    // Enable debug layer
    if (m_Description.EnableDebugLayer)
    {
        DXCall(D3D12GetDebugInterface(IID_PPV_ARGS(&m_DebugLayer)));
        m_DebugLayer->EnableDebugLayer();

        DXCall(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_DXGILayer)));
        m_DXGILayer->EnableLeakTrackingForThread();
    }

    // Create a device with the minimum supported feature level
    ComPtr<ID3D12Device> device = nullptr;
    DXCall(D3D12CreateDevice(m_Adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));

    // Get the max supported feature level
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_12_1
    };

    D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevelInfo = {};
    featureLevelInfo.NumFeatureLevels = _countof(featureLevels);
    featureLevelInfo.pFeatureLevelsRequested = featureLevels;
    DXCall(device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevelInfo, sizeof(featureLevelInfo)));

    // Create the main device with the max supported feature level
    HEXRAY_ASSERT(featureLevelInfo.MaxSupportedFeatureLevel >= D3D_FEATURE_LEVEL_12_0);
    DXCall(D3D12CreateDevice(m_Adapter.Get(), featureLevelInfo.MaxSupportedFeatureLevel, IID_PPV_ARGS(&m_Device)));
    DXCall(m_Device->SetName(L"Main Device"));

    // Get feature options
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureOptions;
    DXCall(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureOptions, sizeof(featureOptions)));
    m_HardwareRayTracingSupported = featureOptions.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;

    if (m_Description.EnableDebugLayer)
    {
        // Set up the message severity levels
        DXCall(m_Device.As(&m_InfoQueue));

        m_InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        m_InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        m_InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

        // Workaround for Windows 11 DX12 Debug Layer
        D3D12_MESSAGE_ID hideMsg[] =
        {
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
        };

        D3D12_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumIDs = _countof(hideMsg);
        filter.DenyList.pIDList = hideMsg;
        m_InfoQueue->AddStorageFilterEntries(&filter);
    }
}

// ------------------------------------------------------------------------------------------------------------------------------------
void GraphicsContext::CreateCommandQueues()
{
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.NodeMask = 0;
    commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    DXCall(m_Device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_GraphicsQueue)));
    DXCall(m_GraphicsQueue->SetName(L"Graphics Queue"));

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        DXCall(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocators[i])));
        DXCall(m_CommandAllocators[i]->SetName(fmt::format(L"Command Allocator {}", i).c_str()));
    }

    DXCall(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&m_CommandList)));
    DXCall(m_CommandList->SetName(L"Main Command List"));
    DXCall(m_CommandList->Close());
}

// ------------------------------------------------------------------------------------------------------------------------------------
void GraphicsContext::CreateDescriptorHeaps()
{
    StandardDescriptorHeapDescription rtvHeapDesc;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Capacity = 1024;

    m_RTVDescriptorHeap = std::make_unique<StandardDescriptorHeap>(rtvHeapDesc, L"RTV Descriptor Heap");

    StandardDescriptorHeapDescription dsvHeapDesc;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Capacity = 1024;

    m_DSVDescriptorHeap = std::make_unique<StandardDescriptorHeap>(dsvHeapDesc, L"DSV Descriptor Heap");

    SegregatedDescriptorHeapDescription resourceHeapDesc;
    resourceHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    resourceHeapDesc.DescriptorTableSizes[ROTexture2D] = 4096;
    resourceHeapDesc.DescriptorTableSizes[ROTextureCube] = 4096;
    resourceHeapDesc.DescriptorTableSizes[ROBuffer] = 1024;
    resourceHeapDesc.DescriptorTableSizes[RWTexture2D] = 2048;
    resourceHeapDesc.DescriptorTableSizes[RWTextureCube] = 2048;
    resourceHeapDesc.DescriptorTableSizes[RWBuffer] = 1024;

    m_ResourceDescriptorHeap = std::make_unique<SegregatedDescriptorHeap>(resourceHeapDesc, L"Resource Descriptor Heap");
}

// ------------------------------------------------------------------------------------------------------------------------------------
void GraphicsContext::CreateSwapChainAndSyncPrimitives()
{
    // Create synchronization objects
    m_FenceValue = 0;
    memset(m_FrameFenceValues, 0, sizeof(m_FrameFenceValues));
    DXCall(m_Device->CreateFence(m_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_FrameFence)));
    DXCall(m_FrameFence->SetName(L"Frame Fence"));

    m_FrameFenceEvent = CreateEvent(0, false, false, 0);

    // Create the swap chain
    uint32_t tearingFeatureValue;
    DXCall(m_Factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearingFeatureValue, sizeof(uint32_t)));
    m_TearingSupported = tearingFeatureValue != 0;

    m_SwapChainWidth = m_Description.Window->GetWidth();
    m_SwapChainHeight = m_Description.Window->GetHeight();

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FRAMES_IN_FLIGHT;
    swapChainDesc.Width = m_SwapChainWidth;
    swapChainDesc.Height = m_SwapChainHeight;
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Flags = m_TearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

    ComPtr<IDXGISwapChain1> swapChain;
    DXCall(m_Factory->CreateSwapChainForHwnd(m_GraphicsQueue.Get(), m_Description.Window->GetWindowHandle(), &swapChainDesc, nullptr, nullptr, &swapChain));
    DXCall(m_Factory->MakeWindowAssociation(m_Description.Window->GetWindowHandle(), DXGI_MWA_NO_ALT_ENTER));

    DXCall(swapChain.As(&m_SwapChain));
    m_BackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
        m_BackBufferRTVs[i] = m_RTVDescriptorHeap->Allocate();

    RecreateRenderTargetViews(swapChainDesc.Width, swapChainDesc.Height);
}

// ------------------------------------------------------------------------------------------------------------------------------------
void GraphicsContext::CreateRootSignatures()
{
    // Root params
    std::vector<D3D12_ROOT_PARAMETER1> rootParams;

    D3D12_ROOT_PARAMETER1& constantBufferParam = rootParams.emplace_back();
    constantBufferParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    constantBufferParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    constantBufferParam.Descriptor.ShaderRegister = 0;
    constantBufferParam.Descriptor.RegisterSpace = 0;
    constantBufferParam.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE;

    std::vector<D3D12_DESCRIPTOR_RANGE1> resourceBindlessTableRanges;

    for (uint32_t descriptorType = 0; descriptorType < DescriptorType::NumDescriptorTypes; descriptorType++)
    {
        bool isReadOnly = descriptorType == ROTexture2D || descriptorType == ROTextureCube || descriptorType == ROBuffer;
        uint32_t registerSpace = 0;

        if (descriptorType == ROTextureCube || descriptorType == RWTextureCube)
        {
            registerSpace = 1;
        }
        else if (descriptorType == ROBuffer || descriptorType == RWBuffer)
        {
            registerSpace = 2;
        }

        D3D12_DESCRIPTOR_RANGE1& range = resourceBindlessTableRanges.emplace_back();
        range.RangeType = isReadOnly ? D3D12_DESCRIPTOR_RANGE_TYPE_SRV : D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        range.RegisterSpace = registerSpace;
        range.BaseShaderRegister = 0;
        range.NumDescriptors = m_ResourceDescriptorHeap->GetDescription().DescriptorTableSizes[descriptorType];
        range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
        range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    }

    D3D12_ROOT_PARAMETER1& resourceBindlessTableParam = rootParams.emplace_back();
    resourceBindlessTableParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    resourceBindlessTableParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    resourceBindlessTableParam.DescriptorTable.NumDescriptorRanges = resourceBindlessTableRanges.size();
    resourceBindlessTableParam.DescriptorTable.pDescriptorRanges = resourceBindlessTableRanges.data();

    // Static samplers
    std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplerDescs;

    D3D12_STATIC_SAMPLER_DESC& pointClampSamplerDesc = staticSamplerDescs.emplace_back();
    pointClampSamplerDesc.ShaderRegister = 0;
    pointClampSamplerDesc.RegisterSpace = 0;
    pointClampSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    pointClampSamplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
    pointClampSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    pointClampSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    pointClampSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    pointClampSamplerDesc.MaxAnisotropy = 1;
    pointClampSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    pointClampSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    pointClampSamplerDesc.MipLODBias = 0.0f;
    pointClampSamplerDesc.MinLOD = 0.0f;
    pointClampSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

    D3D12_STATIC_SAMPLER_DESC& pointWrapSamplerDesc = staticSamplerDescs.emplace_back();
    pointWrapSamplerDesc.ShaderRegister = 1;
    pointWrapSamplerDesc.RegisterSpace = 0;
    pointWrapSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    pointWrapSamplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
    pointWrapSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    pointWrapSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    pointWrapSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    pointWrapSamplerDesc.MaxAnisotropy = 1;
    pointWrapSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    pointWrapSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    pointWrapSamplerDesc.MipLODBias = 0.0f;
    pointWrapSamplerDesc.MinLOD = 0.0f;
    pointWrapSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

    D3D12_STATIC_SAMPLER_DESC& linearClampSamplerDesc = staticSamplerDescs.emplace_back();
    linearClampSamplerDesc.ShaderRegister = 2;
    linearClampSamplerDesc.RegisterSpace = 0;
    linearClampSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    linearClampSamplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
    linearClampSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    linearClampSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    linearClampSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    linearClampSamplerDesc.MaxAnisotropy = 1;
    linearClampSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    linearClampSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    linearClampSamplerDesc.MipLODBias = 0.0f;
    linearClampSamplerDesc.MinLOD = 0.0f;
    linearClampSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

    D3D12_STATIC_SAMPLER_DESC& linearWrapSamplerDesc = staticSamplerDescs.emplace_back();
    linearWrapSamplerDesc.ShaderRegister = 3;
    linearWrapSamplerDesc.RegisterSpace = 0;
    linearWrapSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    linearWrapSamplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
    linearWrapSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    linearWrapSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    linearWrapSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    linearWrapSamplerDesc.MaxAnisotropy = 1;
    linearWrapSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    linearWrapSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    linearWrapSamplerDesc.MipLODBias = 0.0f;
    linearWrapSamplerDesc.MinLOD = 0.0f;
    linearWrapSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

    D3D12_STATIC_SAMPLER_DESC& anisotropicSamplerDesc = staticSamplerDescs.emplace_back();
    anisotropicSamplerDesc.ShaderRegister = 4;
    anisotropicSamplerDesc.RegisterSpace = 0;
    anisotropicSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    anisotropicSamplerDesc.Filter = D3D12_FILTER_COMPARISON_ANISOTROPIC;
    anisotropicSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    anisotropicSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    anisotropicSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    anisotropicSamplerDesc.MaxAnisotropy = D3D12_REQ_MAXANISOTROPY;
    anisotropicSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    anisotropicSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    anisotropicSamplerDesc.MipLODBias = 0.0f;
    anisotropicSamplerDesc.MinLOD = 0.0f;
    anisotropicSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

    D3D12_FEATURE_DATA_ROOT_SIGNATURE rootSigVersionFeatureData;
    rootSigVersionFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

    DXCall(m_Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &rootSigVersionFeatureData, sizeof(rootSigVersionFeatureData)));
    HEXRAY_ASSERT(rootSigVersionFeatureData.HighestVersion == D3D_ROOT_SIGNATURE_VERSION_1_1);

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    rootSignatureDesc.Desc_1_1.NumParameters = rootParams.size();
    rootSignatureDesc.Desc_1_1.pParameters = rootParams.data();
    rootSignatureDesc.Desc_1_1.NumStaticSamplers = staticSamplerDescs.size();
    rootSignatureDesc.Desc_1_1.pStaticSamplers = staticSamplerDescs.data();
    rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> rootSigBlob = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;

    D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &rootSigBlob, &errorBlob);
    HEXRAY_ASSERT_MSG(!(errorBlob && errorBlob->GetBufferSize()), "Root signature serialization failed: {}", (char*)errorBlob->GetBufferPointer());

    DXCall(m_Device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&m_BindlessRootSignature)));
}
