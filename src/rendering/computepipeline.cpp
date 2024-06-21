#include "computepipeline.h"
#include "rendering/graphicscontext.h"

#include <fstream>

// ------------------------------------------------------------------------------------------------------------------------------------
ComputePipeline::ComputePipeline(const ComputePipelineDescription& description, const wchar_t* debugName)
{
    LoadShaderData(description.ShaderFilepath);
    CreatePipelineState(debugName);
}

// ------------------------------------------------------------------------------------------------------------------------------------
ComputePipeline::~ComputePipeline()
{
    delete[] m_ShaderByteCode.pShaderBytecode;
}

// ------------------------------------------------------------------------------------------------------------------------------------
void ComputePipeline::LoadShaderData(const std::filesystem::path& shaderFilepath)
{
    HEXRAY_ASSERT_MSG(std::filesystem::exists(shaderFilepath), "Could not find shader file: {}", shaderFilepath.string());

    std::ifstream ifs(shaderFilepath, std::ios::in | std::ios::binary | std::ios::ate);
    HEXRAY_ASSERT_MSG(ifs, "Could not open shader file: {}", shaderFilepath.string());

    uint32_t fileSize = ifs.tellg();
    m_ShaderByteCode.BytecodeLength = fileSize;
    m_ShaderByteCode.pShaderBytecode = new uint8_t[fileSize];

    ifs.seekg(0, std::ios::beg);
    ifs.read((char*)m_ShaderByteCode.pShaderBytecode, fileSize);
    ifs.close();
}

// ------------------------------------------------------------------------------------------------------------------------------------
void ComputePipeline::CreatePipelineState(const wchar_t* debugName)
{
    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
    desc.CS = m_ShaderByteCode;
    desc.pRootSignature = GraphicsContext::GetInstance()->GetBindlessRootSignature().Get();

    DXCall(GraphicsContext::GetInstance()->GetDevice()->CreateComputePipelineState(&desc, IID_PPV_ARGS(&m_PipelineState)));
    DXCall(m_PipelineState->SetName(debugName));
}
