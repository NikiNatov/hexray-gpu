#include "mesh.h"

#include "rendering/graphicscontext.h"

// ------------------------------------------------------------------------------------------------------------------------------------
Mesh::Mesh(const MeshDescription& description, const wchar_t* debugName)
    : m_Submeshes(description.Submeshes)
{
    HEXRAY_ASSERT(description.Indices.size() && description.Positions.size());
    HEXRAY_ASSERT(description.Normals.empty() || description.Positions.size() == description.Normals.size());
    HEXRAY_ASSERT(description.UVs.empty() || description.Positions.size() == description.UVs.size());
    HEXRAY_ASSERT(description.Tangents.empty() || description.Positions.size() == description.Tangents.size());
    HEXRAY_ASSERT(description.Bitangents.empty() || description.Positions.size() == description.Bitangents.size());

    std::vector<Vertex> vertices;
    vertices.reserve(description.Positions.size());

    for (uint32_t i = 0; i < description.Positions.size(); i++)
    {
        Vertex v;
        v.Position = description.Positions[i];
        v.TexCoord = !description.UVs.empty() ? description.UVs[i] : glm::vec2(0.0f);
        v.Normal = !description.Normals.empty() ? description.Normals[i] : glm::vec3(0.0f);
        v.Tangent = !description.Tangents.empty() ? description.Tangents[i] : glm::vec3(0.0f);
        v.Bitangent = !description.Bitangents.empty() ? description.Bitangents[i] : glm::vec3(0.0f);
        vertices.push_back(std::move(v));
    }

    // Create vertex buffer
    BufferDescription vbDesc;
    vbDesc.ElementCount = vertices.size();
    vbDesc.ElementSize = sizeof(Vertex);
    vbDesc.InitialState = D3D12_RESOURCE_STATE_COMMON;

    m_VertexBuffer = std::make_shared<Buffer>(vbDesc, fmt::format(L"{} Vertex Buffer", debugName).c_str());

    GraphicsContext::GetInstance()->UploadBufferData(m_VertexBuffer.get(), vertices.data());

    // Create index buffer
    BufferDescription ibDesc;
    ibDesc.ElementCount = description.Indices.size();
    ibDesc.ElementSize = sizeof(uint32_t);
    vbDesc.InitialState = D3D12_RESOURCE_STATE_COMMON;

    m_IndexBuffer = std::make_shared<Buffer>(ibDesc, fmt::format(L"{} Index Buffer", debugName).c_str());

    GraphicsContext::GetInstance()->UploadBufferData(m_IndexBuffer.get(), description.Indices.data());

    // Create acceleration structure
    m_AccelerationStructure = GraphicsContext::GetInstance()->BuildBottomLevelAccelerationStructure(this);
}
