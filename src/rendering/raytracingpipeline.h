#pragma once

#include "core/core.h"
#include "rendering/buffer.h"
#include "directx12.h"

struct HitGroup
{
    D3D12_HIT_GROUP_TYPE Type;
    std::wstring Name;
    std::wstring ClosestHitShader;
    std::wstring AnyHitShader;
    std::wstring IntersectionShader;
};

struct RaytracingPipelineDescription
{
    std::string ShaderFilePath;
    uint32_t MaxPayloadSize;
    uint32_t MaxIntersectAttributesSize;
    uint32_t MaxRecursionDepth;
    std::wstring RayGenShader;
    std::vector<std::wstring> MissShaders;
    std::vector<HitGroup> HitGroups;
};

class RaytracingPipeline
{
public:
    RaytracingPipeline(const RaytracingPipelineDescription& description, const wchar_t* debugName = L"Unnamed Raytracing Pipeline");
    ~RaytracingPipeline();
    
    inline uint32_t GetMaxPayloadSize() const { return m_Description.MaxPayloadSize; }
    inline uint32_t GetMaxIntersectAttributesSize() const { return m_Description.MaxIntersectAttributesSize; }
    inline uint32_t GetMaxRecursionDepth() const { return m_Description.MaxRecursionDepth; }
    inline uint32_t GetShaderRecordSize() const { return m_ShaderRecordSize; }

    inline const D3D12_SHADER_BYTECODE& GetShaderByteCode() const { return m_ShaderByteCode; }
    inline const ComPtr<ID3D12StateObject>& GetStateObject() const { return m_StateObject; }
    inline Buffer* GetRayGenTable() const { return m_RayGenTable.get(); }
    inline Buffer* GetHitGroupTable() const { return m_HitGroupTable.get(); }
    inline Buffer* GetMissTableBuffer() const { return m_MissTableBuffer.get(); }
private:
    void LoadShaderData();
    void CreatePipelineState(const wchar_t* debugName);
    void CreateShaderTables(const wchar_t* debugName);
private:
    RaytracingPipelineDescription m_Description;
    D3D12_SHADER_BYTECODE m_ShaderByteCode;
    uint32_t m_ShaderRecordSize;
    ComPtr<ID3D12StateObject> m_StateObject;
    std::unique_ptr<Buffer> m_RayGenTable;
    std::unique_ptr<Buffer> m_HitGroupTable;
    std::unique_ptr<Buffer> m_MissTableBuffer;
};
