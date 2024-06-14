#include "sceneserializer.h"

#include "scene/entity.h"
#include "scene/component.h"
#include "asset/assetmanager.h"

#include <yaml-cpp/yaml.h>
#include <glm.hpp>
#include <fstream>

namespace YAML
{
	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};
}

YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
	return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
	return out;
}

// ------------------------------------------------------------------------------------------------------------------------------------
void SceneSerializer::Serialize(const std::filesystem::path& filepath, const std::shared_ptr<Scene>& scene)
{
	YAML::Emitter out;
	out << YAML::BeginMap;
	out << YAML::Key << "Name" << YAML::Value << scene->GetName();

	const Camera& camera = scene->GetCamera();
	out << YAML::Key << "Camera" << YAML::Value;
	out << YAML::BeginMap;
	out << YAML::Key << "Position" << YAML::Value << camera.GetPosition();
	out << YAML::Key << "Yaw" << YAML::Value << camera.GetYawAngle();
	out << YAML::Key << "Pitch" << YAML::Value << camera.GetPitchAngle();
	out << YAML::Key << "FOV" << YAML::Value << camera.GetPerspectiveFOV();
	out << YAML::EndMap;

	out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

	scene->m_Registry.each([&](auto entityID)
	{
		Entity entity = { entityID, scene.get() };
	
		if (!entity)
			return;
	
		out << YAML::BeginMap;
		out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID();
	
		if (entity.HasComponent<TagComponent>())
		{
			TagComponent& tc = entity.GetComponent<TagComponent>();
			out << YAML::Key << "TagComponent";
			out << YAML::BeginMap;
			out << YAML::Key << "Tag" << YAML::Value << tc.Tag;
			out << YAML::EndMap;
		}
	
		if (entity.HasComponent<SceneHierarchyComponent>())
		{
			SceneHierarchyComponent& shc = entity.GetComponent<SceneHierarchyComponent>();
			out << YAML::Key << "SceneHierarchyComponent";
			out << YAML::BeginMap;
			out << YAML::Key << "Parent" << YAML::Value << shc.Parent;
			out << YAML::Key << "FirstChild" << YAML::Value << shc.FirstChild;
			out << YAML::Key << "NextSibling" << YAML::Value << shc.NextSibling;
			out << YAML::Key << "PreviousSibling" << YAML::Value << shc.PreviousSibling;
			out << YAML::EndMap;
		}
	
		if (entity.HasComponent<TransformComponent>())
		{
			TransformComponent& tc = entity.GetComponent<TransformComponent>();
			out << YAML::Key << "TransformComponent";
			out << YAML::BeginMap;
			out << YAML::Key << "Translation" << YAML::Value << tc.Translation;
			out << YAML::Key << "Rotation" << YAML::Value << tc.Rotation;
			out << YAML::Key << "Scale" << YAML::Value << tc.Scale;
			out << YAML::EndMap;
		}
	
		if (entity.HasComponent<MeshComponent>())
		{
			MeshComponent& mc = entity.GetComponent<MeshComponent>();
			out << YAML::Key << "MeshComponent";
			out << YAML::BeginMap;
			out << YAML::Key << "Mesh" << YAML::Value << mc.Mesh->GetID();
			out << YAML::Key << "Materials" << YAML::Value << YAML::BeginSeq;
			if (mc.OverrideMaterialTable)
			{
				for (const MaterialPtr& material : *mc.OverrideMaterialTable)
				{
					out << YAML::Dec << (material ? material->GetID() : Uuid::Invalid);
				}
			}
			out << YAML::EndSeq;
			out << YAML::EndMap;
		}
	
		if (entity.HasComponent<SkyLightComponent>())
		{
			SkyLightComponent& slc = entity.GetComponent<SkyLightComponent>();
			out << YAML::Key << "SkyLightComponent";
			out << YAML::BeginMap;
			out << YAML::Key << "EnvironmentMap" << YAML::Value << slc.EnvironmentMap->GetID();
			out << YAML::EndMap;
		}
	
		if (entity.HasComponent<DirectionalLightComponent>())
		{
			DirectionalLightComponent& dlc = entity.GetComponent<DirectionalLightComponent>();
			out << YAML::Key << "DirectionalLightComponent";
			out << YAML::BeginMap;
			out << YAML::Key << "Color" << YAML::Value << dlc.Color;
			out << YAML::Key << "Intensity" << YAML::Value << dlc.Intensity;
			out << YAML::EndMap;
		}
	
		if (entity.HasComponent<PointLightComponent>())
		{
			PointLightComponent& plc = entity.GetComponent<PointLightComponent>();
			out << YAML::Key << "PointLightComponent";
			out << YAML::BeginMap;
			out << YAML::Key << "Color" << YAML::Value << plc.Color;
			out << YAML::Key << "Intensity" << YAML::Value << plc.Intensity;
			out << YAML::Key << "AttenuationFactors" << YAML::Value << plc.AttenuationFactors;
			out << YAML::EndMap;
		}
	
		if (entity.HasComponent<SpotLightComponent>())
		{
			SpotLightComponent& slc = entity.GetComponent<SpotLightComponent>();
			out << YAML::Key << "SpotLightComponent";
			out << YAML::BeginMap;
			out << YAML::Key << "Color" << YAML::Value << slc.Color;
			out << YAML::Key << "Direction" << YAML::Value << slc.Direction;
			out << YAML::Key << "ConeAngle" << YAML::Value << slc.ConeAngle;
			out << YAML::Key << "Intensity" << YAML::Value << slc.Intensity;
			out << YAML::Key << "AttenuationFactors" << YAML::Value << slc.AttenuationFactors;
			out << YAML::EndMap;
		}
	
		out << YAML::EndMap;
	});
	
	out << YAML::EndSeq;
	out << YAML::EndMap;
	
	std::ofstream fout(filepath);
	fout << out.c_str();
}

// ------------------------------------------------------------------------------------------------------------------------------------
bool SceneSerializer::Deserialize(const std::filesystem::path& filepath, std::shared_ptr<Scene>& scene)
{
	std::ifstream stream(filepath);
	std::stringstream strStream;

	strStream << stream.rdbuf();

	YAML::Node data = YAML::Load(strStream.str());

	YAML::Node nameNode = data["Name"];
	if (!nameNode)
		return false;

	YAML::Node cameraNode = data["Camera"];
	if (!cameraNode)
		return false;

	std::string sceneName = nameNode.as<std::string>();

	glm::vec3 position = cameraNode["Position"].as<glm::vec3>();
	float yaw = cameraNode["Yaw"].as<float>();
	float pitch = cameraNode["Pitch"].as<float>();
	float fov = cameraNode["FOV"].as<float>();

	Camera camera(fov, 16.0f / 9.0f, position, yaw, pitch);
	scene = std::make_shared<Scene>(sceneName, camera);

	if (YAML::Node entities = data["Entities"])
	{
		for (int64_t it = entities.size() - 1; it >= 0; it--)
		{
			Uuid uuid = entities[it]["Entity"].as<uint64_t>();
			YAML::Node tagComponent = entities[it]["TagComponent"];
			std::string entityName = tagComponent ? tagComponent["Tag"].as<std::string>() : "Unnamed";

			Entity deserializedEntity = scene->CreateEntityFromUUID(uuid, entityName);

			if (YAML::Node sceneNodeComponent = entities[it]["SceneHierarchyComponent"])
			{
				SceneHierarchyComponent& shc = deserializedEntity.GetComponent<SceneHierarchyComponent>();
				shc.Parent = sceneNodeComponent["Parent"].as<uint64_t>();
				shc.FirstChild = sceneNodeComponent["FirstChild"].as<uint64_t>();
				shc.NextSibling = sceneNodeComponent["NextSibling"].as<uint64_t>();
				shc.PreviousSibling = sceneNodeComponent["PreviousSibling"].as<uint64_t>();
			}

			if (YAML::Node transformComponent = entities[it]["TransformComponent"])
			{
				TransformComponent& tc = deserializedEntity.GetComponent<TransformComponent>();
				tc.Translation = transformComponent["Translation"].as<glm::vec3>();
				tc.Rotation = transformComponent["Rotation"].as<glm::vec3>();
				tc.Scale = transformComponent["Scale"].as<glm::vec3>();
			}

			if (YAML::Node meshComponent = entities[it]["MeshComponent"])
			{
				Uuid meshID = meshComponent["Mesh"].as<uint64_t>();

				MeshComponent& mc = deserializedEntity.AddComponent<MeshComponent>();
				mc.Mesh = AssetManager::GetAsset<Mesh>(meshID);

				if (YAML::Node overrideMaterials = meshComponent["Materials"])
				{
					if (overrideMaterials.size())
					{
						mc.OverrideMaterialTable = std::make_shared<MaterialTable>(overrideMaterials.size());
						for (uint32_t i = 0; i < overrideMaterials.size(); i++)
						{
							Uuid materialID = overrideMaterials[i].as<uint64_t>();
							if (materialID != Uuid::Invalid)
							{
								mc.OverrideMaterialTable->SetMaterial(i, AssetManager::GetAsset<Material>(materialID));
							}
						}
					}
				}
			}

			if (YAML::Node skyLightComponent = entities[it]["SkyLightComponent"])
			{
				Uuid textureID = skyLightComponent["EnvironmentMap"].as<uint64_t>();

				SkyLightComponent& slc = deserializedEntity.AddComponent<SkyLightComponent>();
				slc.EnvironmentMap = AssetManager::GetAsset<Texture>(textureID);
			}

			if (YAML::Node dirLightComponent = entities[it]["DirectionalLightComponent"])
			{
				DirectionalLightComponent& dlc = deserializedEntity.AddComponent<DirectionalLightComponent>();
				dlc.Color = dirLightComponent["Color"].as<glm::vec3>();
				dlc.Intensity = dirLightComponent["Intensity"].as<float>();
			}

			if (YAML::Node pointLightComponent = entities[it]["PointLightComponent"])
			{
				PointLightComponent& plc = deserializedEntity.AddComponent<PointLightComponent>();
				plc.Color = pointLightComponent["Color"].as<glm::vec3>();
				plc.Intensity = pointLightComponent["Intensity"].as<float>();
				plc.AttenuationFactors = pointLightComponent["AttenuationFactors"].as<glm::vec3>();
			}

			if (YAML::Node spotLightComponent = entities[it]["SpotLightComponent"])
			{
				SpotLightComponent& slc = deserializedEntity.AddComponent<SpotLightComponent>();
				slc.Color = spotLightComponent["Color"].as<glm::vec3>();
				slc.Direction = spotLightComponent["Direction"].as<glm::vec3>();
				slc.ConeAngle = spotLightComponent["ConeAngle"].as<float>();
				slc.Intensity = spotLightComponent["Intensity"].as<float>();
				slc.AttenuationFactors = spotLightComponent["AttenuationFactors"].as<glm::vec3>();
			}
		}
	}

	return true;
}
