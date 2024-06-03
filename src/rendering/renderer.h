#pragma once

#include "core/core.h"
#include "rendering/texture.h"
#include "rendering/buffer.h"
#include "rendering/raytracingpipeline.h"
#include "rendering/mesh.h"
#include "rendering/camera.h"

#include <glm.hpp>

enum class LightType
{
    DirLight,
    PointLight,
    SpotLight
};

struct Light
{
    glm::vec3 Position;
    glm::vec3 Direction;
    glm::vec3 Color;
    float Intensity;
    float ConeAngle;
    glm::vec3 AttenuationFactors;
    LightType Type;
};

struct MeshInstance
{
    glm::mat4 Transform;
    std::shared_ptr<Mesh> Mesh;
};

struct ResourceBindTable
{
    DescriptorIndex RenderTarget = InvalidDescriptorIndex;
    DescriptorIndex SceneBuffer = InvalidDescriptorIndex;
    DescriptorIndex LightsBuffer = InvalidDescriptorIndex;
    DescriptorIndex MaterialBuffer = InvalidDescriptorIndex;
    DescriptorIndex GeometryBuffer = InvalidDescriptorIndex;
    DescriptorIndex AccelerationStructure = InvalidDescriptorIndex;
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

    void BeginScene(const Camera& camera, const glm::mat4& cameraTransform, const std::shared_ptr<Texture>& environmentMap);
    void SubmitDirectionalLight(const glm::vec3& color, const glm::vec3& direction, float intensity);
    void SubmitPointLight(const glm::vec3& color, const glm::vec3& position, float intensity, const glm::vec3& attenuationFactors);
    void SubmitSpotLight(const glm::vec3& color, const glm::vec3& position, const glm::vec3& direction, float intensity, float coneAngle, const glm::vec3& attenuationFactors);
    void SubmitMesh(const std::shared_ptr<Mesh>& mesh, const glm::mat4& transform);
    void SetViewportSize(uint32_t width, uint32_t height);
    void Render();

    const std::shared_ptr<Texture>& GetFinalImage() const;
private:
    struct SceneConstants
    {
        glm::mat4 ViewMatrix = glm::mat4(1.0f);
        glm::mat4 ProjectionMatrix = glm::mat4(1.0f);
        glm::mat4 InvViewProjMatrix = glm::mat4(1.0f);
        glm::vec3 CameraPosition = glm::vec3(0.0f);
        uint32_t NumLights = 0;
        DescriptorIndex EnvironmentMap = InvalidDescriptorIndex;
    };

    struct MaterialConstants
    {
        glm::vec3 AlbedoColor = glm::vec3(1.0f);
        float Roughness = 0.5f;
        float Metalness = 0.5f;
        DescriptorIndex AlbedoMap = InvalidDescriptorIndex;
        DescriptorIndex NormalMap = InvalidDescriptorIndex;
        DescriptorIndex RoughnessMap = InvalidDescriptorIndex;
        DescriptorIndex MetalnessMap = InvalidDescriptorIndex;
    };

    struct GeometryConstants
    {
        uint32_t MaterialIndex = 0;
        DescriptorIndex VertexBuffer = InvalidDescriptorIndex;
        DescriptorIndex IndexBuffer = InvalidDescriptorIndex;
    };
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