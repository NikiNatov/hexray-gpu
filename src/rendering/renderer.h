#pragma once

#include "core/core.h"
#include "rendering/texture.h"
#include "rendering/buffer.h"
#include "rendering/raytracingpipeline.h"
#include "rendering/mesh.h"
#include "rendering/camera.h"
#include "rendering/shaders/resources.h"

struct MeshInstance
{
    glm::mat4 Transform;
    std::shared_ptr<Mesh> Mesh;
    std::shared_ptr<MaterialTable> OverrideMaterialTable;
};

struct RendererDescription
{
    uint32_t RayRecursionDepth = 3;
};

class Renderer
{
public:
    Renderer(const RendererDescription& description);
    ~Renderer();

    void BeginScene(const Camera& camera, const std::shared_ptr<Texture>& environmentMap);
    void SubmitDirectionalLight(const glm::vec3& color, const glm::vec3& direction, float intensity);
    void SubmitPointLight(const glm::vec3& color, const glm::vec3& position, float intensity, const glm::vec3& attenuationFactors);
    void SubmitSpotLight(const glm::vec3& color, const glm::vec3& position, const glm::vec3& direction, float intensity, float coneAngle, const glm::vec3& attenuationFactors);
    void SubmitMesh(const std::shared_ptr<Mesh>& mesh, const glm::mat4& transform, const std::shared_ptr<MaterialTable>& overrideMaterialTable);
    void SetViewportSize(uint32_t width, uint32_t height);
    void Render();

    const std::shared_ptr<Texture>& GetFinalImage() const;
private:
    RendererDescription m_Description;
    ResourceBindTable m_ResourceBindTable;
    SceneConstants m_SceneConstants;
    uint32_t m_ViewportWidth = 1;
    uint32_t m_ViewportHeight = 1;
    std::vector<Light> m_Lights;
    std::vector<MeshInstance> m_MeshInstances;

    std::shared_ptr<RaytracingPipeline> m_RTPipeline;
    std::shared_ptr<Texture> m_RenderTargets[FRAMES_IN_FLIGHT];
    std::shared_ptr<Buffer> m_SceneBuffers[FRAMES_IN_FLIGHT];
    std::shared_ptr<Buffer> m_LightsBuffers[FRAMES_IN_FLIGHT];
    std::shared_ptr<Buffer> m_MaterialBuffers[FRAMES_IN_FLIGHT];
    std::shared_ptr<Buffer> m_GeometryBuffers[FRAMES_IN_FLIGHT];
    std::shared_ptr<Buffer> m_TopLevelAccelerationStructures[FRAMES_IN_FLIGHT];
};