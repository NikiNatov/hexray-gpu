#include "raytracingpipeline.h"
#include "rendering/graphicscontext.h"

#include <fstream>

// ------------------------------------------------------------------------------------------------------------------------------------
RaytracingPipeline::RaytracingPipeline(const RaytracingPipelineDescription& description, const wchar_t* debugName)
    : m_Description(description)
{
    LoadShaderData();
    CreatePipelineState(debugName);
    CreateShaderTables(debugName);
}

// ------------------------------------------------------------------------------------------------------------------------------------
RaytracingPipeline::~RaytracingPipeline()
{
    delete[] m_ShaderByteCode.pShaderBytecode;
}

// ------------------------------------------------------------------------------------------------------------------------------------
void RaytracingPipeline::LoadShaderData()
{
    HEXRAY_ASSERT_MSG(std::filesystem::exists(m_Description.ShaderFilePath), "Could not find shader file: {}", m_Description.ShaderFilePath);

    std::ifstream ifs(m_Description.ShaderFilePath, std::ios::in | std::ios::binary | std::ios::ate);
    HEXRAY_ASSERT_MSG(ifs, "Could not open shader file: {}", m_Description.ShaderFilePath);

    uint32_t fileSize = ifs.tellg();
    m_ShaderByteCode.BytecodeLength = fileSize;
    m_ShaderByteCode.pShaderBytecode = new uint8_t[fileSize];

    ifs.seekg(0, std::ios::beg);
    ifs.read((char*)m_ShaderByteCode.pShaderBytecode, fileSize);
    ifs.close();
}

// ------------------------------------------------------------------------------------------------------------------------------------
void RaytracingPipeline::CreatePipelineState(const wchar_t* debugName)
{
    CD3DX12_STATE_OBJECT_DESC raytracingPipelineDesc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

    // Initialize DXIL library subobject:
    // Sets which shader library should be used
    CD3DX12_DXIL_LIBRARY_SUBOBJECT* shaderLib = raytracingPipelineDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    shaderLib->SetDXILLibrary(&m_ShaderByteCode);

    // Initialize a global root signature subobject:
    // Sets a root signature shared across all raytracing shaders invoked during a DispatchRays() call. In our case we always use the bindless root signature
    CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT* globalRootSignature = raytracingPipelineDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    globalRootSignature->SetRootSignature(GraphicsContext::GetInstance()->GetBindlessRootSignature().Get());

    // Initialize shader config subobject:
    // Defines the size of the payload and intersection attributes structs
    CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT* shaderConfig = raytracingPipelineDesc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    shaderConfig->Config(m_Description.MaxPayloadSize, m_Description.MaxIntersectAttributesSize);

    // Initialize a pipeline config subobject:
    // Defines the maximum ray recursion depth.
    CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT* pipelineConfig = raytracingPipelineDesc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    pipelineConfig->Config(m_Description.MaxRecursionDepth);

    // Initialize a HitGroup subobjects:
    // Sets the names of the closest hit, any hit and intersection shaders that should run
    // TODO: Find a way to get the names by using shader reflection instead of hardcoding them
    for (const HitGroup& hitGroupDesc : m_Description.HitGroups)
    {
        CD3DX12_HIT_GROUP_SUBOBJECT* hitGroup = raytracingPipelineDesc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
        hitGroup->SetHitGroupExport(hitGroupDesc.Name.c_str());
        hitGroup->SetHitGroupType(hitGroupDesc.Type);

        if (!hitGroupDesc.ClosestHitShader.empty())
            hitGroup->SetClosestHitShaderImport(hitGroupDesc.ClosestHitShader.c_str());

        if (!hitGroupDesc.AnyHitShader.empty())
            hitGroup->SetAnyHitShaderImport(hitGroupDesc.AnyHitShader.c_str());

        if (hitGroupDesc.Type == D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE && !hitGroupDesc.IntersectionShader.empty())
            hitGroup->SetIntersectionShaderImport(hitGroupDesc.IntersectionShader.c_str());
    }

    DXCall(GraphicsContext::GetInstance()->GetDevice()->CreateStateObject(raytracingPipelineDesc, IID_PPV_ARGS(&m_StateObject)));
    DXCall(m_StateObject->SetName(debugName));
}

// ------------------------------------------------------------------------------------------------------------------------------------
void RaytracingPipeline::CreateShaderTables(const wchar_t* debugName)
{
    ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
    DXCall(m_StateObject.As(&stateObjectProperties));

    uint32_t shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    m_ShaderRecordSize = Align(shaderIdentifierSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

    // Create ray gen shader table
    BufferDescription rayGenTableDesc;
    rayGenTableDesc.ElementCount = 1;
    rayGenTableDesc.ElementSize = m_ShaderRecordSize;
    rayGenTableDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
    rayGenTableDesc.InitialState = D3D12_RESOURCE_STATE_GENERIC_READ;

    m_RayGenTable = std::make_unique<Buffer>(rayGenTableDesc, fmt::format(L"{} RayGenShaderTable", debugName).c_str());

    uint8_t* rayGenTableData = (uint8_t*)m_RayGenTable->GetMappedData();

    void* rayGenShaderIdentifier = stateObjectProperties->GetShaderIdentifier(m_Description.RayGenShader.c_str());
    memcpy(rayGenTableData, rayGenShaderIdentifier, shaderIdentifierSize);

    // Create hit group shader table
    BufferDescription hitGroupTableDesc;
    hitGroupTableDesc.ElementCount = m_Description.HitGroups.size();
    hitGroupTableDesc.ElementSize = m_ShaderRecordSize;
    hitGroupTableDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
    hitGroupTableDesc.InitialState = D3D12_RESOURCE_STATE_GENERIC_READ;

    m_HitGroupTable = std::make_unique<Buffer>(hitGroupTableDesc, fmt::format(L"{} HitGroupShaderTable", debugName).c_str());

    uint8_t* hitGroupTableData = (uint8_t*)m_HitGroupTable->GetMappedData();

    for (uint32_t i = 0; i < m_Description.HitGroups.size(); i++)
    {
        uint32_t offset = m_ShaderRecordSize * i;
        void* hitGroupShaderIdentifier = stateObjectProperties->GetShaderIdentifier(m_Description.HitGroups[i].Name.c_str());
        memcpy(hitGroupTableData + offset, hitGroupShaderIdentifier, shaderIdentifierSize);
    }

    // Create miss shader table
    BufferDescription missTableDesc;
    missTableDesc.ElementCount = m_Description.MissShaders.size();
    missTableDesc.ElementSize = m_ShaderRecordSize;
    missTableDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
    missTableDesc.InitialState = D3D12_RESOURCE_STATE_GENERIC_READ;

    m_MissTableBuffer = std::make_unique<Buffer>(missTableDesc, fmt::format(L"{} MissShaderTable", debugName).c_str());

    uint8_t* missTableData = (uint8_t*)m_MissTableBuffer->GetMappedData();

    for (uint32_t i = 0; i < m_Description.MissShaders.size(); i++)
    {
        uint32_t offset = m_ShaderRecordSize * i;
        void* missShaderIdentifier = stateObjectProperties->GetShaderIdentifier(m_Description.MissShaders[i].c_str());
        memcpy(missTableData + offset, missShaderIdentifier, shaderIdentifierSize);
    }
}
