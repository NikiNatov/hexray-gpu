#pragma once
#include "serialization/sceneparser.h"
#include "rendering/texture.h"
#include "rendering/material.h"
#include "rendering/mesh.h"
#include "scene/component.h"

#include <unordered_map>
#include <string>
#include <vector>

class ParsedBlockImpl;
class Scene;
struct RendererDescription;


class DefaultSceneParser : public SceneParser
{
public:
	DefaultSceneParser();
	~DefaultSceneParser();

	bool AddSceneElement(const std::string& className, const std::string& objectName, ParsedBlockImpl& pb);
	virtual bool ResolveFullPath(char* path);
	virtual MaterialPtr FindMaterialByName(const char* name) const;
	virtual TexturePtr FindTextureByName(const char* name) const;
	virtual MeshPtr FindMeshByName(const char* name) const;

	bool Parse(const char* filename, Scene* s, RendererDescription* rendererDesc);
private:
	bool PostParse(const char* filename, std::vector<ParsedBlockImpl>& parsedBlocks);
	void ReplaceRandomNumbers(int srcLine, char line[]);
private:
	Scene* m_Scene;
	RendererDescription* m_RendererDescription;

	std::unordered_map<std::string, MaterialPtr> m_Materials;
	std::unordered_map<std::string, MeshPtr> m_Meshes;
	std::unordered_map<std::string, TexturePtr> m_Textures;
	std::unordered_map<MeshPtr, TransformComponent> m_MeshesTransforms;

	std::string m_SceneRootDirectory;
	int m_CurrentLine;
	bool m_IsParsingObject;
};
