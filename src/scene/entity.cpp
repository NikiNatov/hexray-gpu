#include "entity.h"

#include "scene/component.h"
#include "scene/scene.h"

// ------------------------------------------------------------------------------------------------------------------------------------
Entity::Entity(entt::entity entity, Scene* scene)
	: m_Entity(entity), m_Scene(scene)
{
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Entity::AddChild(Entity entity)
{
	HEXRAY_ASSERT(entity != *this);

	auto& shc = GetComponent<SceneHierarchyComponent>();

	if (!shc.FirstChild)
	{
		shc.FirstChild = entity.GetUUID();
	}
	else
	{
		Entity currentChild = m_Scene->FindEntityByUUID(shc.FirstChild);

		while (currentChild.GetComponent<SceneHierarchyComponent>().NextSibling)
			currentChild = m_Scene->FindEntityByUUID(currentChild.GetComponent<SceneHierarchyComponent>().NextSibling);

		currentChild.GetComponent<SceneHierarchyComponent>().NextSibling = entity.GetUUID();
		entity.GetComponent<SceneHierarchyComponent>().PreviousSibling = currentChild.GetUUID();
		entity.GetComponent<SceneHierarchyComponent>().NextSibling = Uuid(0);
	}

	entity.GetComponent<SceneHierarchyComponent>().Parent = GetUUID();
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Entity::RemoveChild(Entity child)
{
	auto& shc = GetComponent<SceneHierarchyComponent>();
	Entity firstChild = m_Scene->FindEntityByUUID(shc.FirstChild);
	Entity currentChild = firstChild;

	while (currentChild)
	{
		if (currentChild == child)
		{
			if (currentChild == firstChild)
			{
				Entity nextSibling = m_Scene->FindEntityByUUID(currentChild.GetComponent<SceneHierarchyComponent>().NextSibling);
				shc.FirstChild = nextSibling.GetUUID();

				if (nextSibling)
					nextSibling.GetComponent<SceneHierarchyComponent>().PreviousSibling = Uuid(0);
			}
			else
			{
				Entity prev = m_Scene->FindEntityByUUID(currentChild.GetComponent<SceneHierarchyComponent>().PreviousSibling);
				Entity next = m_Scene->FindEntityByUUID(currentChild.GetComponent<SceneHierarchyComponent>().NextSibling);

				if (prev)
					prev.GetComponent<SceneHierarchyComponent>().NextSibling = next.GetUUID();
				if (next)
					next.GetComponent<SceneHierarchyComponent>().PreviousSibling = prev.GetUUID();
			}

			currentChild.GetComponent<SceneHierarchyComponent>().NextSibling = Uuid(0);
			currentChild.GetComponent<SceneHierarchyComponent>().PreviousSibling = Uuid(0);
			currentChild.GetComponent<SceneHierarchyComponent>().Parent = Uuid(0);

			return;
		}

		currentChild = m_Scene->FindEntityByUUID(currentChild.GetComponent<SceneHierarchyComponent>().NextSibling);
	}
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Entity::RemoveParent()
{
	auto& shc = GetComponent<SceneHierarchyComponent>();
	Entity parent = m_Scene->FindEntityByUUID(shc.Parent);

	auto& parentShc = parent.GetComponent<SceneHierarchyComponent>();

	Entity parentFirstChild = m_Scene->FindEntityByUUID(parentShc.FirstChild);
	Entity currentChild = parentFirstChild;

	while (currentChild)
	{
		if (currentChild.GetUUID() == GetUUID())
		{
			if (currentChild == parentFirstChild)
			{
				Entity nextSibling = m_Scene->FindEntityByUUID(currentChild.GetComponent<SceneHierarchyComponent>().NextSibling);
				parentShc.FirstChild = nextSibling.GetUUID();

				if (nextSibling)
					nextSibling.GetComponent<SceneHierarchyComponent>().PreviousSibling = Uuid(0);
			}
			else
			{
				Entity prev = m_Scene->FindEntityByUUID(currentChild.GetComponent<SceneHierarchyComponent>().PreviousSibling);
				Entity next = m_Scene->FindEntityByUUID(currentChild.GetComponent<SceneHierarchyComponent>().NextSibling);

				if (prev)
					prev.GetComponent<SceneHierarchyComponent>().NextSibling = next.GetUUID();
				if (next)
					next.GetComponent<SceneHierarchyComponent>().PreviousSibling = prev.GetUUID();
			}

			break;
		}

		currentChild = m_Scene->FindEntityByUUID(currentChild.GetComponent<SceneHierarchyComponent>().NextSibling);
	}

	shc.Parent = Uuid(0);
}

// ------------------------------------------------------------------------------------------------------------------------------------
bool Entity::IsDescendantOf(Entity entity)
{
	std::queue<Entity> q;

	// Enqueue all the children of the queried entity
	Entity currentChild = m_Scene->FindEntityByUUID(entity.GetComponent<SceneHierarchyComponent>().FirstChild);
	while (currentChild)
	{
		q.push(currentChild);
		currentChild = m_Scene->FindEntityByUUID(currentChild.GetComponent<SceneHierarchyComponent>().NextSibling);
	}

	while (!q.empty())
	{
		Entity child = q.front();
		q.pop();

		if (child == *this)
			return true;

		currentChild = m_Scene->FindEntityByUUID(child.GetComponent<SceneHierarchyComponent>().FirstChild);
		while (currentChild)
		{
			q.push(currentChild);
			currentChild = m_Scene->FindEntityByUUID(currentChild.GetComponent<SceneHierarchyComponent>().NextSibling);
		}
	}

	return false;
}

// ------------------------------------------------------------------------------------------------------------------------------------
Uuid Entity::GetUUID()
{
	if (!*this)
		return Uuid(0);

	return GetComponent<IDComponent>().ID;
}

// -----------------------------------------------------------------------------------------------------------------------------
const std::string& Entity::GetTag()
{
	return GetComponent<TagComponent>().Tag;
}
