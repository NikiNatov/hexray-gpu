#pragma once

#include "core/core.h"
#include "core/uuid.h"

#include <entt/entt.hpp>

class Scene;

class Entity
{
public:
	Entity() = default;
	Entity(entt::entity entity, Scene* scene);
	Entity(const Entity& other) = default;

	void AddChild(Entity entity);
	void RemoveChild(Entity child);
	void RemoveParent();
	bool IsDescendantOf(Entity entity);

	template<typename T, typename... Args>
	T& AddComponent(Args&&... args)
	{
		HEXRAY_ASSERT_MSG(!HasComponent<T>(), "Component already exists!");
		T& component = m_Scene->m_Registry.emplace<T>(m_Entity, std::forward<Args>(args)...);
		return component;
	}

	template<typename T, typename... Args>
	T& AddOrReplaceComponent(Args&&... args)
	{
		T& component = m_Scene->m_Registry.emplace_or_replace<T>(m_Entity, std::forward<Args>(args)...);
		return component;
	}

	template<typename T>
	void RemoveComponent()
	{
		HEXRAY_ASSERT_MSG(HasComponent<T>(), "Component does not exist!");
		m_Scene->m_Registry.remove<T>(m_Entity);
	}

	template<typename T>
	T& GetComponent()
	{
		HEXRAY_ASSERT_MSG(HasComponent<T>(), "Component does not exist!");
		return m_Scene->m_Registry.get<T>(m_Entity);
	}

	template<typename T>
	bool HasComponent()
	{
		return m_Scene->m_Registry.try_get<T>(m_Entity);
	}

	Uuid GetUUID();
	const std::string& GetTag();

	inline operator bool() const { return m_Entity != entt::null; }
	inline operator uint32_t() const { return (uint32_t)m_Entity; }
	inline operator entt::entity() const { return m_Entity; }
	inline bool operator==(const Entity& other) const { return m_Entity == other.m_Entity && m_Scene == other.m_Scene; }
	inline bool operator!= (const Entity& other) const { return !(*this == other); }
private:
	entt::entity m_Entity = entt::null;
	Scene* m_Scene = nullptr;
};
