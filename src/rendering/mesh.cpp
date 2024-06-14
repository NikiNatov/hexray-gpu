#include "mesh.h"

#include "rendering/graphicscontext.h"

// ------------------------------------------------------------------------------------------------------------------------------------
Mesh::Mesh(const MeshDescription& description, const wchar_t* debugName)
    : Asset(AssetType::Mesh), m_Description(description)
{
    HEXRAY_ASSERT(m_Description.Submeshes.size() == m_Description.Materials.size());

    CreateGPU(debugName);
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Mesh::UploadGPUData(const Vertex* vertexData, const uint32_t* indexData, bool keepCPUData)
{
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;

    for (uint32_t i = 0 ; i < m_Description.Submeshes.size(); i++)
    {
        const Submesh& submesh = m_Description.Submeshes[i];

        // Upload vertex data
        GraphicsContext::GetInstance()->UploadBufferData(m_VertexBuffers[i].get(), vertexData + submesh.StartVertex);

        // Create index buffer
        GraphicsContext::GetInstance()->UploadBufferData(m_IndexBuffers[i].get(), indexData + submesh.StartIndex);

        // Create acceleration structure
        m_AccelerationStructures[i] = GraphicsContext::GetInstance()->BuildBottomLevelAccelerationStructure(this, i);

        vertexCount += submesh.VertexCount;
        indexCount += submesh.IndexCount;
    }

    if (keepCPUData)
    {
        m_Vertices.resize(vertexCount);
        memcpy(m_Vertices.data(), vertexData, vertexCount * sizeof(Vertex));

        m_Indices.resize(indexCount);
        memcpy(m_Indices.data(), indexData, indexCount * sizeof(uint32_t));
    }
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Mesh::CreateGPU(const wchar_t* debugName)
{
    m_VertexBuffers.resize(m_Description.Submeshes.size());
    m_IndexBuffers.resize(m_Description.Submeshes.size());
    m_AccelerationStructures.resize(m_Description.Submeshes.size());

    for (uint32_t i = 0; i < m_Description.Submeshes.size(); i++)
    {
        const Submesh& submesh = m_Description.Submeshes[i];

        // Create vertex buffer
        BufferDescription vbDesc;
        vbDesc.ElementCount = submesh.VertexCount;
        vbDesc.ElementSize = sizeof(Vertex);
        vbDesc.InitialState = D3D12_RESOURCE_STATE_COMMON;

        m_VertexBuffers[i] = std::make_shared<Buffer>(vbDesc, fmt::format(L"{} Submesh {} Vertex Buffer", debugName, m_Description.Submeshes.size() - 1).c_str());

        // Create index buffer
        BufferDescription ibDesc;
        ibDesc.ElementCount = submesh.IndexCount;
        ibDesc.ElementSize = sizeof(uint32_t);
        vbDesc.InitialState = D3D12_RESOURCE_STATE_COMMON;

        m_IndexBuffers[i] = std::make_shared<Buffer>(ibDesc, fmt::format(L"{} Submesh {} Index Buffer", debugName, m_Description.Submeshes.size() - 1).c_str());
    }
}
