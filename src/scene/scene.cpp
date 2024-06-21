#include "scene.h"

#include "scene/component.h"

// ------------------------------------------------------------------------------------------------------------------------------------
Scene::Scene(const std::string& name)
    : m_Name(name)
{
}

// ------------------------------------------------------------------------------------------------------------------------------------
Scene::Scene(const std::string& name, const Camera& camera)
    : m_Name(name), m_Camera(camera)
{
}

// ------------------------------------------------------------------------------------------------------------------------------------
Entity Scene::CreateEntity(const std::string& name)
{
    return CreateEntityFromUUID(Uuid(), name);
}

// ------------------------------------------------------------------------------------------------------------------------------------
Entity Scene::CreateEntityFromUUID(Uuid uuid, const std::string& name)
{
    Entity entity(m_Registry.create(), this);
    entity.AddComponent<IDComponent>(uuid);
    entity.AddComponent<TagComponent>(name);
    entity.AddComponent<TransformComponent>();
    entity.AddComponent<SceneHierarchyComponent>();
    m_EntitiesByID[uuid] = entity;

    return entity;
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Scene::DeleteEntity(Entity entity)
{
    auto& shc = entity.GetComponent<SceneHierarchyComponent>();
    Entity currentChild = FindEntityByUUID(shc.FirstChild);

    // Delete all children recursively first
    while (currentChild)
    {
        Entity nextSibling = FindEntityByUUID(currentChild.GetComponent<SceneHierarchyComponent>().NextSibling);
        DeleteEntity(currentChild);
        currentChild = nextSibling;
    }

    // Fix links between neighbouring entities
    Entity parent = FindEntityByUUID(shc.Parent);
    if (parent && FindEntityByUUID(parent.GetComponent<SceneHierarchyComponent>().FirstChild) == entity)
    {
        Entity nextSibling = FindEntityByUUID(shc.NextSibling);
        parent.GetComponent<SceneHierarchyComponent>().FirstChild = nextSibling.GetUUID();

        if (nextSibling)
            nextSibling.GetComponent<SceneHierarchyComponent>().PreviousSibling = Uuid(0);
    }
    else
    {
        Entity prev = FindEntityByUUID(shc.PreviousSibling);
        Entity next = FindEntityByUUID(shc.NextSibling);

        if (prev)
            prev.GetComponent<SceneHierarchyComponent>().NextSibling = next.GetUUID();
        if (next)
            next.GetComponent<SceneHierarchyComponent>().PreviousSibling = prev.GetUUID();
    }

    m_EntitiesByID.erase(entity.GetUUID());
    m_Registry.destroy((entt::entity)entity);
}

// ------------------------------------------------------------------------------------------------------------------------------------
Entity Scene::FindEntityByUUID(Uuid uuid)
{
    auto it = m_EntitiesByID.find(uuid);

    if (it == m_EntitiesByID.end())
        return {};

    return it->second;
}

// ------------------------------------------------------------------------------------------------------------------------------------
Entity Scene::FindEntityByName(const std::string& name)
{
    auto view = m_Registry.view<TagComponent>();
    for (auto entity : view)
    {
        auto& tag = view.get<TagComponent>(entity);
        if (tag.Tag == name)
            return { entity, this };
    }

    return {};
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Scene::OnUpdate(float dt)
{
    m_Camera.OnUpdate(dt);
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Scene::OnRender(const std::shared_ptr<Renderer>& renderer)
{
    // Sky light
    std::shared_ptr<Texture> environmentMap = nullptr;
    {
        auto view = m_Registry.view<SkyLightComponent>();
        entt::entity skyLightEntity = view.back();

        if (skyLightEntity == entt::null)
        {
            HEXRAY_WARNING("Trying to render a scene with no sky lighting");
        }
        else
        {
            auto& slc = view.get<SkyLightComponent>(skyLightEntity);

            if (slc.EnvironmentMap)
                environmentMap = slc.EnvironmentMap;
        }
    }

    renderer->BeginScene(m_Camera, environmentMap);

    // Submit directional lights
    {
        auto view = m_Registry.view<DirectionalLightComponent, TransformComponent>();
        for (auto entity : view)
        {
            auto [dlc, tc] = view.get<DirectionalLightComponent, TransformComponent>(entity);
            renderer->SubmitDirectionalLight(dlc.Color, glm::normalize(-tc.Translation), dlc.Intensity);
        }
    }

    // Submit point lights
    {
        auto view = m_Registry.view<PointLightComponent, TransformComponent>();
        for (auto entity : view)
        {
            auto [plc, tc] = view.get<PointLightComponent, TransformComponent>(entity);
            renderer->SubmitPointLight(plc.Color, tc.Translation, plc.Intensity, plc.AttenuationFactors);
        }
    }

    // Submit spot lights
    {
        auto view = m_Registry.view<SpotLightComponent, TransformComponent>();
        for (auto entity : view)
        {
            auto [slc, tc] = view.get<SpotLightComponent, TransformComponent>(entity);
            renderer->SubmitSpotLight(slc.Color, tc.Translation, glm::normalize(slc.Direction), slc.Intensity, glm::radians(slc.ConeAngle), slc.AttenuationFactors);
        }
    }

    // Submit meshes
    {
        auto view = m_Registry.view<MeshComponent, TransformComponent, SceneHierarchyComponent>();
        for (auto entity : view)
        {
            auto [mc, tc, shc] = view.get<MeshComponent, TransformComponent, SceneHierarchyComponent>(entity);

            if (mc.Mesh)
            {
                if (shc.Parent)
                {
                    Entity currentParent = FindEntityByUUID(shc.Parent);
                    auto accumulatedTransform = currentParent.GetComponent<TransformComponent>().GetTransform();

                    while (currentParent.GetComponent<SceneHierarchyComponent>().Parent)
                    {
                        currentParent = FindEntityByUUID(currentParent.GetComponent<SceneHierarchyComponent>().Parent);
                        accumulatedTransform = currentParent.GetComponent<TransformComponent>().GetTransform() * accumulatedTransform;
                    }

                    renderer->SubmitMesh(mc.Mesh, accumulatedTransform * tc.GetTransform(), mc.OverrideMaterialTable);
                }
                else
                    renderer->SubmitMesh(mc.Mesh, tc.GetTransform(), mc.OverrideMaterialTable);
            }
        }
    }

    renderer->Render();
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Scene::OnViewportResize(uint32_t width, uint32_t height)
{
    m_Camera.SetViewportSize(width, height);
}
