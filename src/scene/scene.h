#pragma once

#include "core/core.h"
#include "scene/entity.h"
#include "rendering/renderer.h"

#include <entt/entt.hpp>

class Scene
{
    friend class Entity;
    friend class SceneSerializer;
public:
    Scene(const std::string& name);
    Scene(const std::string& name, const Camera& camera);

    Entity CreateEntity(const std::string& name = "Unnamed Entity");
    Entity CreateEntityFromUUID(Uuid uuid, const std::string& name = "Unnamed Entity");
    void DeleteEntity(Entity entity);
    Entity FindEntityByUUID(Uuid uuid);
    Entity FindEntityByName(const std::string& name);

    void OnUpdate(float dt);
    void OnRender(const std::shared_ptr<Renderer>& renderer);
    void OnViewportResize(uint32_t width, uint32_t height);

    inline const std::string& GetName() { return m_Name; }
    inline Camera& GetCamera() { return m_Camera; }
private:
    std::string m_Name;
    entt::registry m_Registry;
    std::unordered_map<Uuid, Entity> m_EntitiesByID;
    Camera m_Camera;
};