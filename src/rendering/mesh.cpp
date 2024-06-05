#include "mesh.h"

#include "rendering/graphicscontext.h"

// ------------------------------------------------------------------------------------------------------------------------------------
MeshDescription MeshDescription::CreateQuad()
{
    MeshDescription quadMeshDesc;
    quadMeshDesc.Indices = { 0, 1, 2, 2, 3, 0 };
    quadMeshDesc.Positions = {
        { -1.0f, -1.0f, 0.0f },
        {  1.0f, -1.0f, 0.0f },
        {  1.0f,  1.0f, 0.0f },
        { -1.0f,  1.0f, 0.0f }
    };
    quadMeshDesc.UVs = {
        { 0.0f, 1.0f },
        { 1.0f, 1.0f },
        { 1.0f, 0.0f },
        { 0.0f, 0.0f }
    };
    quadMeshDesc.Submeshes = { SubmeshDescription{ 0, 4, 0, 6 } };
    return quadMeshDesc;
}

// ------------------------------------------------------------------------------------------------------------------------------------
Mesh::Mesh(const MeshDescription& description, const wchar_t* debugName)
{
    HEXRAY_ASSERT(description.Submeshes.size() == description.Materials.size());
    HEXRAY_ASSERT(description.Indices.size() && description.Positions.size());
    HEXRAY_ASSERT(description.Normals.empty() || description.Positions.size() == description.Normals.size());
    HEXRAY_ASSERT(description.UVs.empty() || description.Positions.size() == description.UVs.size());
    HEXRAY_ASSERT(description.Tangents.empty() || description.Positions.size() == description.Tangents.size());
    HEXRAY_ASSERT(description.Bitangents.empty() || description.Positions.size() == description.Bitangents.size());

    struct Vertex
    {
        glm::vec3 Position;
        glm::vec2 TexCoord;
        glm::vec3 Normal;
        glm::vec3 Tangent;
        glm::vec3 Bitangent;
    };

    std::vector<Vertex> vertices;
    vertices.reserve(description.Positions.size());

    for (uint32_t i = 0; i < description.Positions.size(); i++)
    {
        Vertex& v = vertices.emplace_back();
        v.Position = description.Positions[i];
        v.TexCoord = !description.UVs.empty() ? description.UVs[i] : glm::vec2(0.0f);
        v.Normal = !description.Normals.empty() ? description.Normals[i] : glm::vec3(0.0f);
        v.Tangent = !description.Tangents.empty() ? description.Tangents[i] : glm::vec3(0.0f);
        v.Bitangent = !description.Bitangents.empty() ? description.Bitangents[i] : glm::vec3(0.0f);
    }

    // Create submeshes
    m_Submeshes.reserve(description.Submeshes.size());

    for (const SubmeshDescription& submeshDesc : description.Submeshes)
    {
        Submesh& submesh = m_Submeshes.emplace_back();
        submesh.Material = description.Materials[submeshDesc.MaterialIndex];

        // Create vertex buffer
        BufferDescription vbDesc;
        vbDesc.ElementCount = submeshDesc.VertexCount;
        vbDesc.ElementSize = sizeof(Vertex);
        vbDesc.InitialState = D3D12_RESOURCE_STATE_COMMON;

        submesh.VertexBuffer = std::make_shared<Buffer>(vbDesc, fmt::format(L"{} Submesh {} Vertex Buffer", debugName, m_Submeshes.size() - 1).c_str());
        GraphicsContext::GetInstance()->UploadBufferData(submesh.VertexBuffer.get(), vertices.data() + submeshDesc.StartVertex);

        // Create index buffer
        BufferDescription ibDesc;
        ibDesc.ElementCount = submeshDesc.IndexCount;
        ibDesc.ElementSize = sizeof(uint32_t);
        vbDesc.InitialState = D3D12_RESOURCE_STATE_COMMON;

        submesh.IndexBuffer = std::make_shared<Buffer>(ibDesc, fmt::format(L"{} Submesh {} Index Buffer", debugName, m_Submeshes.size() - 1).c_str());
        GraphicsContext::GetInstance()->UploadBufferData(submesh.IndexBuffer.get(), description.Indices.data() + submeshDesc.StartIndex);

        // Create acceleration structure
        submesh.AccelerationStructure = GraphicsContext::GetInstance()->BuildBottomLevelAccelerationStructure(submesh);
    }
}
