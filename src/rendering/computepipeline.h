#pragma once

#include "core/core.h"
#include "directx12.h"

struct ComputePipelineDescription
{
    std::filesystem::path ShaderFilepath;
};

class ComputePipeline
{
public:
    ComputePipeline(const ComputePipelineDescription& description, const wchar_t* debugName = L"Unnamed Compute Pipeline");
    ~ComputePipeline();

    inline const D3D12_SHADER_BYTECODE& GetShaderByteCode() const { return m_ShaderByteCode; }
    inline const ComPtr<ID3D12PipelineState>& GetPipelineState() const { return m_PipelineState; }
private:
    void LoadShaderData(const std::filesystem::path& shaderFilepath);
    void CreatePipelineState(const wchar_t* debugName);
private:
    D3D12_SHADER_BYTECODE m_ShaderByteCode;
    ComPtr<ID3D12PipelineState> m_PipelineState;
};
