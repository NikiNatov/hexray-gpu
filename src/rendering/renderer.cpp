#include "renderer.h"
#include "core/application.h"
#include "rendering/graphicscontext.h"
#include "rendering/defaultresources.h"

// ------------------------------------------------------------------------------------------------------------------------------------
Renderer::Renderer(const RendererDescription& description)
    : m_Description(description)
{
    // Create raytracing pipeline
    RaytracingPipelineDescription pipelineDesc;
    pipelineDesc.ShaderFilePath = Application::GetInstance()->GetExecutablePath().parent_path() / "shaderdata.cso";
    pipelineDesc.MaxRecursionDepth = m_Description.RayRecursionDepth;
    pipelineDesc.MaxPayloadSize = sizeof(float) * 4 + sizeof(uint32_t);
    pipelineDesc.MaxIntersectAttributesSize = sizeof(float) * 2;
    pipelineDesc.RayGenShader = L"RayGenShader";
    pipelineDesc.MissShaders = {
        L"MissShader_Radiance", L"MissShader_Shadow"
    };
    pipelineDesc.HitGroups = {
        // TODO: Set correct hit groups for each material model when we add support
        HitGroup { D3D12_HIT_GROUP_TYPE_TRIANGLES, L"LambertColorHitGroup", L"ClosestHitShader_Lambert", L"", L"" },
        HitGroup { D3D12_HIT_GROUP_TYPE_TRIANGLES, L"PhongColorHitGroup", L"ClosestHitShader_Phong", L"", L"" },
        HitGroup { D3D12_HIT_GROUP_TYPE_TRIANGLES, L"PBRColorHitGroup", L"ClosestHitShader_Lambert", L"", L"" }
    };

    m_RTPipeline = std::make_shared<RaytracingPipeline>(pipelineDesc, L"Triangle Raytracing Pipeline");

    // Create scene buffers
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        BufferDescription sceneBufferDesc;
        sceneBufferDesc.ElementCount = 1;
        sceneBufferDesc.ElementSize = sizeof(SceneConstants);
        sceneBufferDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

        m_SceneBuffers[i] = std::make_shared<Buffer>(sceneBufferDesc, fmt::format(L"Scene Buffer {}", i).c_str());
    }
}

// ------------------------------------------------------------------------------------------------------------------------------------
Renderer::~Renderer()
{
    // Make sure we finished executing commands on the GPU so that we can release resources safely
    GraphicsContext::GetInstance()->WaitForGPU();
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Renderer::BeginScene(const Camera& camera, const std::shared_ptr<Texture>& environmentMap)
{
    uint32_t currentFrameIdx = GraphicsContext::GetInstance()->GetBackBufferIndex();

    m_SceneConstants.ViewMatrix = camera.GetViewMatrix();
    m_SceneConstants.ProjectionMatrix = camera.GetProjection();
    m_SceneConstants.InvProjectionMatrix = glm::inverse(m_SceneConstants.ProjectionMatrix);
    m_SceneConstants.InvViewMatrix = glm::inverse(m_SceneConstants.ViewMatrix);
    m_SceneConstants.CameraPosition = camera.GetPosition();
    m_SceneConstants.NumLights = 0;
    
    m_ResourceBindTable.EnvironmentMap = environmentMap ? environmentMap->GetSRV() : DefaultResources::BlackTextureCube->GetSRV();

    m_Lights.clear();
    m_MeshInstances.clear();
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Renderer::SubmitDirectionalLight(const glm::vec3& color, const glm::vec3& direction, float intensity)
{
    Light& light = m_Lights.emplace_back();
    light.Type = LightType::DirLight;
    light.Color = { color.r, color.g, color.b };
    light.Direction = { direction.x, direction.y, direction.z };
    light.Intensity = intensity;

    m_SceneConstants.NumLights++;
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Renderer::SubmitPointLight(const glm::vec3& color, const glm::vec3& position, float intensity, const glm::vec3& attenuationFactors)
{
    Light& light = m_Lights.emplace_back();
    light.Type = LightType::PointLight;
    light.Color = { color.r, color.g, color.b };
    light.Position = { position.x, position.y, position.z };
    light.Intensity = intensity;
    light.AttenuationFactors = attenuationFactors;

    m_SceneConstants.NumLights++;
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Renderer::SubmitSpotLight(const glm::vec3& color, const glm::vec3& position, const glm::vec3& direction, float intensity, float coneAngle, const glm::vec3& attenuationFactors)
{
    Light& light = m_Lights.emplace_back();
    light.Type = LightType::SpotLight;
    light.Color = { color.r, color.g, color.b };
    light.Position = { position.x, position.y, position.z };
    light.Direction = { direction.x, direction.y, direction.z };
    light.Intensity = intensity;
    light.ConeAngle = coneAngle;
    light.AttenuationFactors = attenuationFactors;

    m_SceneConstants.NumLights++;
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Renderer::SubmitMesh(const std::shared_ptr<Mesh>& mesh, const glm::mat4& transform, const std::shared_ptr<MaterialTable>& overrideMaterialTable)
{
    if (!mesh)
        return;

    MeshInstance& instance = m_MeshInstances.emplace_back();
    instance.Transform = transform;
    instance.Mesh = mesh;
    instance.OverrideMaterialTable = overrideMaterialTable;
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Renderer::SetViewportSize(uint32_t width, uint32_t height)
{
    m_ViewportWidth = width;
    m_ViewportHeight = height;
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Renderer::Render()
{
    if (m_MeshInstances.empty())
        return;

    uint32_t currentFrameIndex = GraphicsContext::GetInstance()->GetBackBufferIndex();

    // Update Render target
    std::shared_ptr<Texture>& renderTarget = m_RenderTargets[currentFrameIndex];

    if (!renderTarget || renderTarget->GetWidth() != m_ViewportWidth || renderTarget->GetHeight() != m_ViewportHeight)
    {
        TextureDescription rtDesc;
        rtDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        rtDesc.Width = m_ViewportWidth;
        rtDesc.Height = m_ViewportHeight;
        rtDesc.MipLevels = 1;
        rtDesc.InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        rtDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        renderTarget = std::make_shared<Texture>(rtDesc, fmt::format(L"Render Target {}", currentFrameIndex).c_str());
    }

    m_ResourceBindTable.RenderTarget = renderTarget->GetUAV(0);

    // Update Scene buffer
    std::shared_ptr<Buffer> sceneBuffer = m_SceneBuffers[currentFrameIndex];

    memcpy(sceneBuffer->GetMappedData(), &m_SceneConstants, sizeof(SceneConstants));
    m_ResourceBindTable.SceneBuffer = sceneBuffer->GetSRV();

    // Update Lights buffer
    if (!m_Lights.empty())
    {
        std::shared_ptr<Buffer>& lightsBuffer = m_LightsBuffers[currentFrameIndex];

        if (!lightsBuffer || lightsBuffer->GetElementCount() != m_Lights.size())
        {
            BufferDescription lightsBufferDesc;
            lightsBufferDesc.ElementCount = m_Lights.size();
            lightsBufferDesc.ElementSize = sizeof(Light);
            lightsBufferDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

            lightsBuffer = std::make_shared<Buffer>(lightsBufferDesc, fmt::format(L"Lights Buffer {}", currentFrameIndex).c_str());
        }

        memcpy(lightsBuffer->GetMappedData(), m_Lights.data(), m_Lights.size() * sizeof(Light));

        m_ResourceBindTable.LightsBuffer = lightsBuffer->GetSRV();
    }
    else
    {
        m_ResourceBindTable.LightsBuffer = InvalidDescriptorIndex;
    }

    // Update Material buffer
    std::shared_ptr<Buffer>& materialBuffer = m_MaterialBuffers[currentFrameIndex];

    std::vector<MaterialConstants> materials;
    materials.reserve(m_MeshInstances.size());

    for (const MeshInstance& instance : m_MeshInstances)
    {
        for (uint32_t i = 0; i < instance.Mesh->GetSubmeshes().size(); i++)
        {
            uint32_t materialIndex = instance.Mesh->GetSubmesh(i).MaterialIndex;
            MaterialPtr overrideMaterial = instance.OverrideMaterialTable ? instance.OverrideMaterialTable->GetMaterial(materialIndex) : nullptr;
            MaterialPtr meshMaterial = instance.Mesh->GetMaterial(i);

            std::shared_ptr<Material> material = overrideMaterial ? overrideMaterial : meshMaterial;

            MaterialConstants& materialConstants = materials.emplace_back();
            material->GetProperty(MaterialPropertyType::AlbedoColor, materialConstants.AlbedoColor);

            TexturePtr albedoMap = nullptr;
            if (material->GetTexture(MaterialTextureType::Albedo, albedoMap))
                materialConstants.AlbedoMap = albedoMap ? albedoMap->GetSRV() : InvalidDescriptorIndex;

            TexturePtr normalMap = nullptr;
            if (material->GetTexture(MaterialTextureType::Normal, normalMap))
                materialConstants.NormalMap = normalMap ? normalMap->GetSRV() : InvalidDescriptorIndex;

            if (material->GetType() == MaterialType::Phong)
            {
                material->GetProperty(MaterialPropertyType::SpecularColor, materialConstants.SpecularColor);
                material->GetProperty(MaterialPropertyType::Shininess, materialConstants.Shininess);
            }
            else if (material->GetType() == MaterialType::PBR)
            {
                material->GetProperty(MaterialPropertyType::Roughness, materialConstants.Roughness);
                material->GetProperty(MaterialPropertyType::Metalness, materialConstants.Metalness);

                TexturePtr roughnessMap = nullptr;
                if (material->GetTexture(MaterialTextureType::Roughness, roughnessMap))
                    materialConstants.RoughnessMap = roughnessMap ? roughnessMap->GetSRV() : InvalidDescriptorIndex;

                TexturePtr metalnessMap = nullptr;
                if (material->GetTexture(MaterialTextureType::Metalness, metalnessMap))
                    materialConstants.MetalnessMap = metalnessMap ? metalnessMap->GetSRV() : InvalidDescriptorIndex;
            }
        }
    }

    if (!materialBuffer || materialBuffer->GetElementCount() != materials.size())
    {
        BufferDescription materialBufferDesc;
        materialBufferDesc.ElementCount = materials.size();
        materialBufferDesc.ElementSize = sizeof(MaterialConstants);
        materialBufferDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

        materialBuffer = std::make_shared<Buffer>(materialBufferDesc, fmt::format(L"Material Buffer {}", currentFrameIndex).c_str());
    }

    memcpy(materialBuffer->GetMappedData(), materials.data(), materials.size() * sizeof(MaterialConstants));
    m_ResourceBindTable.MaterialBuffer = materialBuffer->GetSRV();

    // Update Geometry buffer
    std::shared_ptr<Buffer>& geometryBuffer = m_GeometryBuffers[currentFrameIndex];

    std::vector<GeometryConstants> geometries;
    geometries.reserve(m_MeshInstances.size());

    for (const MeshInstance& instance : m_MeshInstances)
    {
        for (uint32_t i = 0; i < instance.Mesh->GetSubmeshes().size(); i++)
        {
            GeometryConstants& geometry = geometries.emplace_back();
            geometry.MaterialIndex = geometries.size() - 1;
            geometry.VertexBuffer = instance.Mesh->GetVertexBuffer(i)->GetSRV();
            geometry.IndexBuffer = instance.Mesh->GetIndexBuffer(i)->GetSRV();
        }
    }

    if (!geometryBuffer || geometryBuffer->GetElementCount() != geometries.size())
    {
        BufferDescription geometryBufferDesc;
        geometryBufferDesc.ElementCount = geometries.size();
        geometryBufferDesc.ElementSize = sizeof(GeometryConstants);
        geometryBufferDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

        geometryBuffer = std::make_shared<Buffer>(geometryBufferDesc, fmt::format(L"Geometry Buffer {}", currentFrameIndex).c_str());
    }

    memcpy(geometryBuffer->GetMappedData(), geometries.data(), geometries.size() * sizeof(GeometryConstants));
    m_ResourceBindTable.GeometryBuffer = geometryBuffer->GetSRV();

    // Update Top-level Acceleration structure
    m_TopLevelAccelerationStructures[currentFrameIndex] = GraphicsContext::GetInstance()->BuildTopLevelAccelerationStructure(m_MeshInstances);
    m_ResourceBindTable.AccelerationStructure = m_TopLevelAccelerationStructures[currentFrameIndex]->GetSRV();

    // Render
    GraphicsContext::GetInstance()->DispatchRays(m_ViewportWidth, m_ViewportHeight, m_ResourceBindTable, m_RTPipeline.get());
}

// ------------------------------------------------------------------------------------------------------------------------------------
const std::shared_ptr<Texture>& Renderer::GetFinalImage() const
{
    uint32_t currentFrameIndex = GraphicsContext::GetInstance()->GetBackBufferIndex();
    return m_RenderTargets[currentFrameIndex];
}
