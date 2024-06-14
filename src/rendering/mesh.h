#pragma once

#include "core/core.h"
#include "rendering/buffer.h"
#include "rendering/material.h"
#include "rendering/resources_fwd.h"
#include "asset/asset.h"

#include <glm.hpp>

struct Vertex
{
    glm::vec3 Position;
    glm::vec2 TexCoord;
    glm::vec3 Normal;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

struct Submesh
{
    uint32_t StartVertex;
    uint32_t VertexCount;
    uint32_t StartIndex;
    uint32_t IndexCount;
    uint32_t MaterialIndex;
};

struct MeshDescription
{
    std::shared_ptr<MaterialTable> MaterialTable;
    std::vector<Submesh> Submeshes;
};

class Mesh : public Asset
{
public:
    Mesh(const MeshDescription& description, const wchar_t* debugName = L"Unnamed Mesh");

    void UploadGPUData(const Vertex* vertexData, const uint32_t* indexData, bool keepCPUData = false);

    inline const std::vector<Vertex>& GetVertices() const { return m_Vertices; }
    inline const std::vector<uint32_t>& GetIndices() const { return m_Indices; }
    inline const std::shared_ptr<MaterialTable>& GetMaterialTable() const { return m_Description.MaterialTable; }
    inline const std::vector<Submesh>& GetSubmeshes() const { return m_Description.Submeshes; }

    inline const Submesh& GetSubmesh(uint32_t submeshIndex) const { return m_Description.Submeshes[submeshIndex]; }
    inline const MaterialPtr& GetMaterial(uint32_t submeshIndex) const { return m_Description.MaterialTable->GetMaterial(GetSubmesh(submeshIndex).MaterialIndex); }
    inline const BufferPtr& GetVertexBuffer(uint32_t submeshIndex) const { return m_VertexBuffers[submeshIndex]; }
    inline const BufferPtr& GetIndexBuffer(uint32_t submeshIndex) const { return m_IndexBuffers[submeshIndex]; }
    inline const BufferPtr& GetAccelerationStructure(uint32_t submeshIndex) const { return m_AccelerationStructures[submeshIndex]; }
private:
    void CreateGPU(const wchar_t* debugName = L"Unnamed Mesh");
private:
    MeshDescription m_Description;
    std::vector<BufferPtr> m_VertexBuffers;
    std::vector<BufferPtr> m_IndexBuffers;
    std::vector<BufferPtr> m_AccelerationStructures;
    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;
};
