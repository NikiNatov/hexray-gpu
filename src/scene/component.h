#pragma once

#include "core/core.h"
#include "core/uuid.h"
#include "scene/entity.h"
#include "rendering/mesh.h"
#include "rendering/texture.h"
#include "rendering/camera.h"
#include "serialization/parsedblock.h"

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

struct BaseComponent : public HObject
{
};

struct IDComponent : public BaseComponent
{
	Uuid ID;

	IDComponent() = default;
	IDComponent(const IDComponent& other) = default;
	IDComponent(Uuid uuid)
		: ID(uuid) {}
};

struct SceneHierarchyComponent : public BaseComponent
{
	Uuid Parent = Uuid(0);
	Uuid FirstChild = Uuid(0);
	Uuid PreviousSibling = Uuid(0);
	Uuid NextSibling = Uuid(0);

	SceneHierarchyComponent() = default;
	SceneHierarchyComponent(const SceneHierarchyComponent& other) = default;
};

struct TagComponent : public BaseComponent
{
	std::string Tag = "";

	TagComponent() = default;
	TagComponent(const TagComponent& other) = default;
	TagComponent(const std::string& tag)
		: Tag(tag) {}
};

struct TransformComponent : public BaseComponent
{
	glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
	glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
	glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

	TransformComponent() = default;
	TransformComponent(const TransformComponent& other) = default;
	TransformComponent(const glm::vec3& translation)
		: Translation(translation) {}

	glm::mat4 GetTransform()
	{
		glm::mat4 translation = glm::translate(glm::mat4(1.0f), Translation);
		glm::mat4 scale = glm::scale(glm::mat4(1.0f), Scale);
		glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), Rotation.z, { 0.0f, 0.0f, 1.0f }) *
			glm::rotate(glm::mat4(1.0f), Rotation.y, { 0.0f, 1.0f, 0.0f }) *
			glm::rotate(glm::mat4(1.0f), Rotation.x, { 1.0f, 0.0f, 0.0f });

		return translation * rotation * scale;
	}
};

struct MeshComponent : public BaseComponent
{
	std::shared_ptr<Mesh> MeshObject = nullptr;

	MeshComponent() = default;
	MeshComponent(const MeshComponent& other) = default;
	MeshComponent(const std::shared_ptr<Mesh>& mesh)
		: MeshObject(mesh) {}

	virtual void Serialize(ParsedBlock& pb)
	{
		pb.GetProperty("geometry", MeshObject);

		MaterialPtr material;
		pb.GetProperty("shader", material);

		TexturePtr bump;
		pb.GetProperty("bump", bump);

		material->SetNormalMap(bump);

		MeshObject->SetMaterial(0, material);
	}
};

struct SkyLightComponent : public BaseComponent
{
	std::shared_ptr<Texture> EnvironmentMap = nullptr;

	SkyLightComponent() = default;
	SkyLightComponent(const SkyLightComponent& other) = default;
	SkyLightComponent(const std::shared_ptr<Texture>& environmentMap)
		: EnvironmentMap(environmentMap) {}
};

struct LightComponent : public BaseComponent
{
	glm::vec3 Color = glm::vec3(1.0f);
	float Intensity = 1.0f;

	LightComponent() = default;
	LightComponent(const glm::vec3& color, float intensity)
		: Color(color), Intensity(intensity) {}

	virtual void Serialize(ParsedBlock& pb)
	{
		pb.GetProperty("color", Color);
		pb.GetProperty("power", Intensity);
	}
};

struct DirectionalLightComponent : public LightComponent
{
	DirectionalLightComponent() = default;
	DirectionalLightComponent(const glm::vec3& color, float intensity)
		: LightComponent(color, intensity) {}
};

struct PointLightComponent : public LightComponent
{
	glm::vec3 AttenuationFactors = { 1.0f, 1.0f, 1.0f };

	PointLightComponent() = default;
	PointLightComponent(const glm::vec3& color, float intensity, const glm::vec3& attenuationFactors = glm::vec3(1.0f, 1.0f, 1.0f))
		: LightComponent(color, intensity), AttenuationFactors(attenuationFactors) {}
};

struct SpotLightComponent : public LightComponent
{
	glm::vec3 Direction = glm::vec3(0.0f);
	float ConeAngle = 30.0f;
	glm::vec3 AttenuationFactors = { 1.0f, 1.0f, 1.0f };

	SpotLightComponent() = default;
	SpotLightComponent(const SpotLightComponent& other) = default;
	SpotLightComponent(const glm::vec3& color, const glm::vec3& direction, float coneAngle, float intensity, const glm::vec3& attenuationFactors = glm::vec3(1.0f, 1.0f, 1.0f))
		: LightComponent(color, intensity), Direction(direction), ConeAngle(coneAngle), AttenuationFactors(attenuationFactors) {}
};