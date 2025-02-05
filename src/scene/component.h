#pragma once

#include "core/core.h"
#include "core/uuid.h"
#include "scene/entity.h"
#include "rendering/mesh.h"
#include "rendering/texture.h"
#include "rendering/camera.h"

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

struct IDComponent
{
	Uuid ID;

	IDComponent() = default;
	IDComponent(const IDComponent& other) = default;
	IDComponent(Uuid uuid)
		: ID(uuid) {}
};

struct SceneHierarchyComponent
{
	Uuid Parent = Uuid(0);
	Uuid FirstChild = Uuid(0);
	Uuid PreviousSibling = Uuid(0);
	Uuid NextSibling = Uuid(0);

	SceneHierarchyComponent() = default;
	SceneHierarchyComponent(const SceneHierarchyComponent& other) = default;
};

struct TagComponent
{
	std::string Tag = "";

	TagComponent() = default;
	TagComponent(const TagComponent& other) = default;
	TagComponent(const std::string& tag)
		: Tag(tag) {}
};

struct TransformComponent
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
		glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(Rotation.z), {0.0f, 0.0f, 1.0f}) *
			glm::rotate(glm::mat4(1.0f), glm::radians(Rotation.y), { 0.0f, 1.0f, 0.0f }) *
			glm::rotate(glm::mat4(1.0f), glm::radians(Rotation.x), { 1.0f, 0.0f, 0.0f });

		return translation * rotation * scale;
	}
};

struct MeshComponent
{
	MeshPtr Mesh = nullptr;
	std::shared_ptr<MaterialTable> OverrideMaterialTable = nullptr;

	MeshComponent() = default;
	MeshComponent(const MeshComponent& other) = default;
	MeshComponent(const MeshPtr& mesh)
		: Mesh(mesh) {}
};

struct SkyLightComponent
{
	TexturePtr EnvironmentMap = nullptr;

	SkyLightComponent() = default;
	SkyLightComponent(const SkyLightComponent& other) = default;
	SkyLightComponent(const TexturePtr& environmentMap)
		: EnvironmentMap(environmentMap) {}
};

struct DirectionalLightComponent
{
	glm::vec3 Color = glm::vec3(1.0f);
	float Intensity = 1.0f;

	DirectionalLightComponent() = default;
	DirectionalLightComponent(const glm::vec3& color, float intensity)
		: Color(color), Intensity(intensity) {}
};

struct PointLightComponent
{
	glm::vec3 Color = glm::vec3(1.0f);
	float Intensity = 1.0f;
	glm::vec3 AttenuationFactors = { 1.0f, 1.0f, 1.0f };

	PointLightComponent() = default;
	PointLightComponent(const glm::vec3& color, float intensity, const glm::vec3& attenuationFactors = glm::vec3(1.0f, 1.0f, 1.0f))
		: Color(color), Intensity(intensity), AttenuationFactors(attenuationFactors) {}
};

struct SpotLightComponent
{
	glm::vec3 Color = glm::vec3(1.0f);
	glm::vec3 Direction = glm::vec3(0.0f);
	float ConeAngleMin = 30.0f;
	float ConeAngleMax = 40.0f;
	float Intensity = 1.0f;
	glm::vec3 AttenuationFactors = { 1.0f, 1.0f, 1.0f };

	SpotLightComponent() = default;
	SpotLightComponent(const SpotLightComponent& other) = default;
	SpotLightComponent(const glm::vec3& color, const glm::vec3& direction, float coneAngleMin, float coneAngleMax, float intensity, const glm::vec3& attenuationFactors = glm::vec3(1.0f, 1.0f, 1.0f))
		: Color(color), Direction(direction), ConeAngleMin(coneAngleMin), ConeAngleMax(coneAngleMax), Intensity(intensity), AttenuationFactors(attenuationFactors) {}
};