#pragma once

#include "core/core.h"
#include "rendering/buffer.h"
#include "rendering/material.h"
#include "rendering/resources_fwd.h"

#include <glm.hpp>

struct SubmeshDescription
{
    uint32_t StartVertex;
    uint32_t VertexCount;
    uint32_t StartIndex;
    uint32_t IndexCount;
    uint32_t MaterialIndex;
};

struct Submesh
{
    BufferPtr VertexBuffer;
    BufferPtr IndexBuffer;
    MaterialPtr Material;
    BufferPtr AccelerationStructure;
};

struct MeshDescription
{
    std::vector<glm::vec3> Positions;
    std::vector<glm::vec2> UVs;
    std::vector<glm::vec3> Normals;
    std::vector<glm::vec3> Tangents;
    std::vector<glm::vec3> Bitangents;
    std::vector<uint32_t> Indices;
    std::vector<MaterialPtr> Materials;
    std::vector<SubmeshDescription> Submeshes;
};

class Mesh
{
public:
    Mesh() = default;
    Mesh(const MeshDescription& description, const wchar_t* debugName = L"Unnamed Mesh");

    void SetMaterial(uint32_t submeshIndex, const MaterialPtr& material) { m_Submeshes[submeshIndex].Material = material; }

    inline uint32_t GetSubmeshCount() const { return m_Submeshes.size(); }
    inline const BufferPtr& GetVertexBuffer(uint32_t submeshIndex) const { return m_Submeshes[submeshIndex].VertexBuffer; }
    inline const BufferPtr& GetIndexBuffer(uint32_t submeshIndex) const { return m_Submeshes[submeshIndex].IndexBuffer; }
    inline const MaterialPtr& GetMaterial(uint32_t submeshIndex) const { return m_Submeshes[submeshIndex].Material; }
    inline const BufferPtr& GetAccelerationStructure(uint32_t submeshIndex) const { return m_Submeshes[submeshIndex].AccelerationStructure; }
private:
    void CreateGPU(const MeshDescription& description, const wchar_t* debugName = L"Unnamed Mesh");
private:
    std::vector<Submesh> m_Submeshes;
};
