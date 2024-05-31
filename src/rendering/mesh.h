#pragma once

#include "core/core.h"
#include "rendering/buffer.h"

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
};

struct MeshDescription
{
    std::vector<glm::vec3> Positions;
    std::vector<glm::vec2> UVs;
    std::vector<glm::vec3> Normals;
    std::vector<glm::vec3> Tangents;
    std::vector<glm::vec3> Bitangents;
    std::vector<uint32_t> Indices;
    std::vector<Submesh> Submeshes;
};

class Mesh
{
public:
    Mesh(const MeshDescription& description, const wchar_t* debugName = L"Unnamed Mesh");

    inline const std::vector<Submesh>& GetSubmeshes() const { return m_Submeshes; }
    inline const std::shared_ptr<Buffer>& GetVertexBuffer() const { return m_VertexBuffer; }
    inline const std::shared_ptr<Buffer>& GetIndexBuffer() const { return m_IndexBuffer; }
    inline const std::shared_ptr<Buffer>& GetAccelerationStructure() const { return m_AccelerationStructure; }
private:
    std::vector<Submesh> m_Submeshes;
    std::shared_ptr<Buffer> m_VertexBuffer;
    std::shared_ptr<Buffer> m_IndexBuffer;
    std::shared_ptr<Buffer> m_AccelerationStructure;
};