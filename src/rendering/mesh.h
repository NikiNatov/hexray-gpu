#pragma once

#include "core/core.h"
#include "rendering/buffer.h"
#include "rendering/material.h"

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
    std::shared_ptr<Buffer> VertexBuffer;
    std::shared_ptr<Buffer> IndexBuffer;
    std::shared_ptr<Material> Material;
    std::shared_ptr<Buffer> AccelerationStructure;
};

struct MeshDescription
{
    std::vector<glm::vec3> Positions;
    std::vector<glm::vec2> UVs;
    std::vector<glm::vec3> Normals;
    std::vector<glm::vec3> Tangents;
    std::vector<glm::vec3> Bitangents;
    std::vector<uint32_t> Indices;
    std::vector<std::shared_ptr<Material>> Materials;
    std::vector<SubmeshDescription> Submeshes;

    static MeshDescription CreateQuad();
};

class Mesh
{
public:
    Mesh(const MeshDescription& description, const wchar_t* debugName = L"Unnamed Mesh");

    inline uint32_t GetSubmeshCount() const { return m_Submeshes.size(); }
    inline const std::shared_ptr<Buffer>& GetVertexBuffer(uint32_t submeshIndex) const { return m_Submeshes[submeshIndex].VertexBuffer; }
    inline const std::shared_ptr<Buffer>& GetIndexBuffer(uint32_t submeshIndex) const { return m_Submeshes[submeshIndex].IndexBuffer; }
    inline const std::shared_ptr<Material>& GetMaterial(uint32_t submeshIndex) const { return m_Submeshes[submeshIndex].Material; }
    inline const std::shared_ptr<Buffer>& GetAccelerationStructure(uint32_t submeshIndex) const { return m_Submeshes[submeshIndex].AccelerationStructure; }
private:
    std::vector<Submesh> m_Submeshes;
};