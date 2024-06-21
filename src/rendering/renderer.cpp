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
    pipelineDesc.MaxPayloadSize = sizeof(ColorRayPayload);
    pipelineDesc.MaxIntersectAttributesSize = sizeof(glm::vec2);
    pipelineDesc.RayGenShader = L"RayGenShader";
    pipelineDesc.MissShaders = {
        L"MissShader_Color", L"MissShader_Shadow"
    };
    pipelineDesc.HitGroups = {
        HitGroup { D3D12_HIT_GROUP_TYPE_TRIANGLES, L"ColorHitGroup", L"ClosestHitShader_Color", L"", L"" },
    };

    m_RTPipeline = std::make_shared<RaytracingPipeline>(pipelineDesc, L"Triangle Raytracing Pipeline");

    // Create post FX pipelines
    ComputePipelineDescription bloomDownsamplePipelineDesc;
    bloomDownsamplePipelineDesc.ShaderFilepath = Application::GetInstance()->GetExecutablePath().parent_path() / "bloomdownsample.cso";
    m_BloomDownsamplePipeline = std::make_shared<ComputePipeline>(bloomDownsamplePipelineDesc, L"Bloom Downsample PSO");

    ComputePipelineDescription bloomUpsamplePipelineDesc;
    bloomUpsamplePipelineDesc.ShaderFilepath = Application::GetInstance()->GetExecutablePath().parent_path() / "bloomupsample.cso";
    m_BloomUpsamplePipeline = std::make_shared<ComputePipeline>(bloomUpsamplePipelineDesc, L"Bloom Upsample PSO");

    ComputePipelineDescription bloomCompositePipelineDesc;
    bloomCompositePipelineDesc.ShaderFilepath = Application::GetInstance()->GetExecutablePath().parent_path() / "bloomcomposite.cso";
    m_BloomCompositePipeline = std::make_shared<ComputePipeline>(bloomCompositePipelineDesc, L"Bloom Composite PSO");

    ComputePipelineDescription tonemapPipelineDesc;
    tonemapPipelineDesc.ShaderFilepath = Application::GetInstance()->GetExecutablePath().parent_path() / "tonemap.cso";
    m_TonemapPipeline = std::make_shared<ComputePipeline>(tonemapPipelineDesc, L"Tonemap PSO");

    // Create per frame resources
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        RecreateTextures(i);

        BufferDescription sceneBufferDesc;
        sceneBufferDesc.ElementCount = 1;
        sceneBufferDesc.ElementSize = sizeof(SceneConstants);
        sceneBufferDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

        m_SceneBuffers[i] = std::make_shared<Buffer>(sceneBufferDesc, fmt::format(L"Scene Buffer {}", i).c_str());
    }

    m_SceneConstants.FrameIndex = 1;
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
    m_SceneConstants.InvProjMatrix = glm::inverse(m_SceneConstants.ProjectionMatrix);
    m_SceneConstants.InvViewMatrix = glm::inverse(m_SceneConstants.ViewMatrix);
    m_SceneConstants.CameraPosition = camera.GetPosition();
    m_SceneConstants.NumLights = 0;

    m_CameraExposure = camera.GetExposure();

    if (camera.HasMoved())
        m_SceneConstants.FrameIndex = 1;
    
    m_ResourceBindTable.EnvironmentMapIndex = environmentMap ? environmentMap->GetSRV() : DefaultResources::BlackTextureCube->GetSRV();

    m_Lights.clear();
    m_MeshInstances.clear();
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Renderer::SubmitDirectionalLight(const glm::vec3& color, const glm::vec3& direction, float intensity)
{
    Light& light = m_Lights.emplace_back();
    light.LightType = LightType::DirLight;
    light.Color = { color.r, color.g, color.b };
    light.Direction = { direction.x, direction.y, direction.z };
    light.Intensity = intensity;

    m_SceneConstants.NumLights++;
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Renderer::SubmitPointLight(const glm::vec3& color, const glm::vec3& position, float intensity, const glm::vec3& attenuationFactors)
{
    Light& light = m_Lights.emplace_back();
    light.LightType = LightType::PointLight;
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
    light.LightType = LightType::SpotLight;
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
    if (m_ViewportWidth == width && m_ViewportHeight == height)
        return;

    m_ViewportWidth = width;
    m_ViewportHeight = height;
    m_SceneConstants.FrameIndex = 1;
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Renderer::Render()
{
    if (m_MeshInstances.empty())
        return;

    uint32_t currentFrameIndex = GraphicsContext::GetInstance()->GetBackBufferIndex();

    // Update Render target
    std::shared_ptr<Texture>& renderTarget = m_RenderTargets[currentFrameIndex];

    if (renderTarget->GetWidth() != m_ViewportWidth || renderTarget->GetHeight() != m_ViewportWidth)
    {
        RecreateTextures(currentFrameIndex);
    }
    
    m_ResourceBindTable.RenderTargetIndex = renderTarget->GetUAV(0);

    std::shared_ptr<Texture>& prevFrameRenderTarget = m_RenderTargets[currentFrameIndex == 0 ? FRAMES_IN_FLIGHT - 1 : currentFrameIndex - 1];
    m_ResourceBindTable.PrevFrameRenderTargetIndex = prevFrameRenderTarget->GetUAV(0);

    // Update Scene buffer
    std::shared_ptr<Buffer> sceneBuffer = m_SceneBuffers[currentFrameIndex];

    memcpy(sceneBuffer->GetMappedData(), &m_SceneConstants, sizeof(SceneConstants));
    m_ResourceBindTable.SceneBufferIndex = sceneBuffer->GetSRV();

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

        m_ResourceBindTable.LightsBufferIndex = lightsBuffer->GetSRV();
    }
    else
    {
        m_ResourceBindTable.LightsBufferIndex = InvalidDescriptorIndex;
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
            materialConstants.MaterialType = (uint32_t)material->GetType();
            material->GetProperty(MaterialPropertyType::AlbedoColor, materialConstants.AlbedoColor);
            material->GetProperty(MaterialPropertyType::EmissiveColor, materialConstants.EmissiveColor);
            material->GetProperty(MaterialPropertyType::EmissionPower, materialConstants.EmissivePower);

            TexturePtr albedoMap = nullptr;
            if (material->GetTexture(MaterialTextureType::Albedo, albedoMap))
                materialConstants.AlbedoMapIndex = albedoMap ? albedoMap->GetSRV() : InvalidDescriptorIndex;

            TexturePtr normalMap = nullptr;
            if (material->GetTexture(MaterialTextureType::Normal, normalMap))
                materialConstants.NormalMapIndex = normalMap ? normalMap->GetSRV() : InvalidDescriptorIndex;

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
                    materialConstants.RoughnessMapIndex = roughnessMap ? roughnessMap->GetSRV() : InvalidDescriptorIndex;

                TexturePtr metalnessMap = nullptr;
                if (material->GetTexture(MaterialTextureType::Metalness, metalnessMap))
                    materialConstants.MetalnessMapIndex = metalnessMap ? metalnessMap->GetSRV() : InvalidDescriptorIndex;
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
    m_ResourceBindTable.MaterialBufferIndex = materialBuffer->GetSRV();

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
            geometry.VertexBufferIndex = instance.Mesh->GetVertexBuffer(i)->GetSRV();
            geometry.IndexBufferIndex = instance.Mesh->GetIndexBuffer(i)->GetSRV();
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
    m_ResourceBindTable.GeometryBufferIndex = geometryBuffer->GetSRV();

    // Update Top-level Acceleration structure
    m_TopLevelAccelerationStructures[currentFrameIndex] = GraphicsContext::GetInstance()->BuildTopLevelAccelerationStructure(m_MeshInstances);
    m_ResourceBindTable.AccelerationStructureIndex = m_TopLevelAccelerationStructures[currentFrameIndex]->GetSRV();

    // Render scene
    GraphicsContext::GetInstance()->DispatchRays(m_ViewportWidth, m_ViewportHeight, m_ResourceBindTable, m_RTPipeline.get());
    GraphicsContext::GetInstance()->AddUAVBarrier(renderTarget.get());

    // Render PostFX
    ApplyBloom();
    ApplyTonemapping();

    m_SceneConstants.FrameIndex++;
}

// ------------------------------------------------------------------------------------------------------------------------------------
const std::shared_ptr<Texture>& Renderer::GetFinalImage() const
{
    uint32_t currentFrameIndex = GraphicsContext::GetInstance()->GetBackBufferIndex();
    return m_FinalOutputTexture[currentFrameIndex];
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Renderer::RecreateTextures(uint32_t frameIndex)
{
    TextureDescription rtDesc;
    rtDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    rtDesc.Width = m_ViewportWidth;
    rtDesc.Height = m_ViewportHeight;
    rtDesc.MipLevels = 1;
    rtDesc.InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    rtDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    m_RenderTargets[frameIndex] = std::make_shared<Texture>(rtDesc, fmt::format(L"Render Target {}", frameIndex).c_str());

    uint32_t maxBloomMips = glm::log2((float)glm::max(m_ViewportWidth, m_ViewportHeight)) + 1;

    TextureDescription bloomTextureDesc;
    bloomTextureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    bloomTextureDesc.Width = m_ViewportWidth;
    bloomTextureDesc.Height = m_ViewportHeight;
    bloomTextureDesc.MipLevels = std::min(maxBloomMips, m_Description.BloomDownsampleSteps);
    bloomTextureDesc.InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    bloomTextureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    m_BloomTextures[frameIndex] = std::make_shared<Texture>(bloomTextureDesc, fmt::format(L"Bloom Texture {}", frameIndex).c_str());

    TextureDescription bloomCompositeTextureDesc;
    bloomCompositeTextureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    bloomCompositeTextureDesc.Width = m_ViewportWidth;
    bloomCompositeTextureDesc.Height = m_ViewportHeight;
    bloomCompositeTextureDesc.MipLevels = 1;
    bloomCompositeTextureDesc.InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    bloomCompositeTextureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    m_BloomCompositeTextures[frameIndex] = std::make_shared<Texture>(bloomCompositeTextureDesc, fmt::format(L"Bloom Composite Texture {}", frameIndex).c_str());

    TextureDescription finalOutputTextureDesc;
    finalOutputTextureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    finalOutputTextureDesc.Width = m_ViewportWidth;
    finalOutputTextureDesc.Height = m_ViewportHeight;
    finalOutputTextureDesc.MipLevels = 1;
    finalOutputTextureDesc.InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    finalOutputTextureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    m_FinalOutputTexture[frameIndex] = std::make_shared<Texture>(finalOutputTextureDesc, fmt::format(L"Final Output {}", frameIndex).c_str());
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Renderer::ApplyBloom()
{
    if (!m_Description.EnableBloom)
        return;

    GraphicsContext* gfxContext = GraphicsContext::GetInstance();
    uint32_t currentFrameIndex = gfxContext->GetBackBufferIndex();

    std::shared_ptr<Texture>& sceneTexture = m_RenderTargets[currentFrameIndex];
    std::shared_ptr<Texture>& bloomTexture = m_BloomTextures[currentFrameIndex];
    std::shared_ptr<Texture>& bloomCompositeTexture = m_BloomCompositeTextures[currentFrameIndex];

    const uint32_t THREAD_COUNT_X = 32;
    const uint32_t THREAD_COUNT_Y = 32;

    // Downsample phase
    for (uint32_t mip = 0; mip < bloomTexture->GetMipLevels(); mip++)
    {
        m_ResourceBindTable.BloomDownsampleConstants.SourceMipLevel = mip == 0 ? mip : mip - 1;
        m_ResourceBindTable.BloomDownsampleConstants.SourceTextureIndex = mip == 0 ? sceneTexture->GetSRV() : bloomTexture->GetSRV();
        m_ResourceBindTable.BloomDownsampleConstants.DownsampledMipLevel = mip;
        m_ResourceBindTable.BloomDownsampleConstants.DownsampledTextureIndex = bloomTexture->GetUAV(mip);

        uint32_t downsampledMipWidth = bloomTexture->GetWidth() >> mip;
        uint32_t downsampledMipHeight = bloomTexture->GetHeight() >> mip;

        uint threadGroupCountX = (downsampledMipWidth + THREAD_COUNT_X - 1) / THREAD_COUNT_X;
        uint threadGroupCountY = (downsampledMipHeight + THREAD_COUNT_Y - 1) / THREAD_COUNT_Y;

        gfxContext->DispatchComputeShader(threadGroupCountX, threadGroupCountY, 1, m_ResourceBindTable, m_BloomDownsamplePipeline.get());
        gfxContext->AddUAVBarrier(bloomTexture.get());
    }

    // Upsample phase
    for (int32_t mip = bloomTexture->GetMipLevels() - 1; mip >= 1; mip--)
    {
        m_ResourceBindTable.BloomUpsampleConstants.FilterRadius = 0.005f;
        m_ResourceBindTable.BloomUpsampleConstants.SourceMipLevel = mip;
        m_ResourceBindTable.BloomUpsampleConstants.SourceTextureIndex = bloomTexture->GetSRV();
        m_ResourceBindTable.BloomUpsampleConstants.UpsampledTextureIndex = bloomTexture->GetUAV(mip - 1);

        uint32_t upsampledMipWidth = bloomTexture->GetWidth() >> (mip - 1);
        uint32_t upsampledMipHeight = bloomTexture->GetHeight() >> (mip - 1);

        uint threadGroupCountX = (upsampledMipWidth + THREAD_COUNT_X - 1) / THREAD_COUNT_X;
        uint threadGroupCountY = (upsampledMipHeight + THREAD_COUNT_Y - 1) / THREAD_COUNT_Y;

        gfxContext->DispatchComputeShader(threadGroupCountX, threadGroupCountY, 1, m_ResourceBindTable, m_BloomUpsamplePipeline.get());
        gfxContext->AddUAVBarrier(bloomTexture.get());
    }

    // Composite phase
    m_ResourceBindTable.BloomCompositeConstants.BloomStrength = m_Description.BloomStrength;
    m_ResourceBindTable.BloomCompositeConstants.BloomTextureIndex = bloomTexture->GetSRV();
    m_ResourceBindTable.BloomCompositeConstants.SceneTextureIndex = sceneTexture->GetSRV();
    m_ResourceBindTable.BloomCompositeConstants.OutputTextureIndex = bloomCompositeTexture->GetUAV(0);

    uint threadGroupCountX = (bloomCompositeTexture->GetWidth() + THREAD_COUNT_X - 1) / THREAD_COUNT_X;
    uint threadGroupCountY = (bloomCompositeTexture->GetHeight() + THREAD_COUNT_Y - 1) / THREAD_COUNT_Y;

    gfxContext->DispatchComputeShader(threadGroupCountX, threadGroupCountY, 1, m_ResourceBindTable, m_BloomCompositePipeline.get());
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Renderer::ApplyTonemapping()
{
    GraphicsContext* gfxContext = GraphicsContext::GetInstance();
    uint32_t currentFrameIndex = gfxContext->GetBackBufferIndex();

    std::shared_ptr<Texture>& inputTexture = m_Description.EnableBloom ? m_BloomCompositeTextures[currentFrameIndex] : m_RenderTargets[currentFrameIndex];
    std::shared_ptr<Texture>& finalOutputTexture = m_FinalOutputTexture[currentFrameIndex];

    const uint32_t THREAD_COUNT_X = 32;
    const uint32_t THREAD_COUNT_Y = 32;

    m_ResourceBindTable.TonemapConstants.Gamma = 2.2f;
    m_ResourceBindTable.TonemapConstants.CameraExposure = m_CameraExposure;
    m_ResourceBindTable.TonemapConstants.InputTextureIndex = inputTexture->GetSRV();
    m_ResourceBindTable.TonemapConstants.OutputTextureIndex = finalOutputTexture->GetUAV(0);

    uint threadGroupCountX = (finalOutputTexture->GetWidth() + THREAD_COUNT_X - 1) / THREAD_COUNT_X;
    uint threadGroupCountY = (finalOutputTexture->GetHeight() + THREAD_COUNT_Y - 1) / THREAD_COUNT_Y;

    gfxContext->DispatchComputeShader(threadGroupCountX, threadGroupCountY, 1, m_ResourceBindTable, m_TonemapPipeline.get());
}