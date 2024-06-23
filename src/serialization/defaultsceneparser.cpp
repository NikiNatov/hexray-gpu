#include "defaultsceneparser.h"
#include "core/utils.h"
#include "core/application.h"
#include "scene/component.h"
#include "scene/scene.h"
#include "serialization/parsedblockimpl.h"
#include "asset/assetimporter.h"
#include "asset/assetmanager.h"
#include "asset/assetserializer.h"
#include "rendering/defaultresources.h"

#include <vector>
#include <cstdarg>
#include <random>


DefaultSceneParser::DefaultSceneParser()
{
	m_IsParsingObject = false;
	m_Scene = nullptr;
	m_SceneRootDirectory = "data/";
}

DefaultSceneParser::~DefaultSceneParser()
{
}

bool DefaultSceneParser::AddSceneElement(const std::string& className, const std::string& objectName, ParsedBlockImpl& pb)
{
	TextureImportOptions noCompressNoMipMapTexOptions;
	noCompressNoMipMapTexOptions.Compress = false;
	noCompressNoMipMapTexOptions.GenerateMips = false;

	MeshImportOptions noFlipMeshOptions;
	noFlipMeshOptions.ConvertToLeftHanded = false;

	if (className == "GlobalSettings")
	{
		int frameWidth, frameHeight;
		pb.GetProperty("frameWidth", frameWidth);
		pb.GetProperty("frameHeight", frameHeight);
		pb.GetProperty("maxTraceDepth", m_RendererDescription->RayRecursionDepth);
		//pb.GetProperty("gi", m_RendererDescription->EnableGlobalIllumination, false);
		//pb.GetProperty("ambientLight", m_RendererDescription->AmbientLight);
		//pb.GetProperty("tonemap", m_RendererDescription->EnableTonemap, false);

		m_RendererDescription->EnableACESTonemap = false;

		Window* window = Application::GetInstance()->GetWindow();

		// Utillity to compare easily results side by side
		auto hexrayHandle = FindWindowA(nullptr, "heX-Ray");
		if (hexrayHandle)
		{
			RECT hexrayRect;
			GetWindowRect(hexrayHandle, &hexrayRect);

			MoveWindow(window->GetWindowHandle(), hexrayRect.left - frameWidth, hexrayRect.top, hexrayRect.right - hexrayRect.left, hexrayRect.bottom - hexrayRect.top, false);

			DestroyWindow(hexrayHandle);
		}
		else
		{
			window->Resize(frameWidth, frameHeight);
		}
	}

	if (className == "Camera")
	{
		auto& camera = m_Scene->GetCamera();
		pb.GetRequiredProperty("pos", camera.m_Position);
		pb.GetProperty("aspectRatio", camera.m_AspectRatio, 1e-6);
		pb.GetProperty("fov", camera.m_PerspectiveFOV, 0.0001, 179);
		pb.GetProperty("yaw", camera.m_YawAngle);
		pb.GetProperty("pitch", camera.m_PitchAngle, -90, 90);
		//pb.GetProperty("roll", m_Roll);
		//pb.GetProperty("fNumber", &fNumber, 0.5, 128.0);
		//pb.GetProperty("numSamples", &numSamples, 1);
		//pb.GetProperty("focalPlaneDist", &focalPlaneDist, 1e-3, 1e+6);
		//pb.GetProperty("dof", &dof);
		//pb.GetProperty("autoFocus", &autoFocus);
		//pb.GetProperty("stereoSeparation", &stereoSeparation, 0.0);

		camera.m_MovementSpeed = 1130.f;
		camera.RecalculateProjection();
		camera.RecalculateView();
		return true;
	}

	// Textures
	//if (className == "CheckerTexture") return new CheckerTexture;
	//if (className == "Heightfield") return new Heightfield;
	//if (className == "Bumps") return new Bumps;
	//if (className == "Const") return new Const;
	if (className == "BitmapTexture" || className == "BumpTexture")
	{
		char filename[256];
		if (!pb.GetFilenameProp("file", filename))
			pb.RequiredProp("file");

		m_Textures[objectName] = AssetManager::GetAsset<Texture>(AssetImporter::ImportTextureAsset(filename, noCompressNoMipMapTexOptions));
		return true;
	}

	if (className == "CubemapEnvironment")
	{
		char folder[256];
		if (!pb.GetFilenameProp("folder", folder))
			pb.RequiredProp("folder");

		Entity sky = m_Scene->CreateEntity(objectName);
		sky.AddComponent<SkyLightComponent>().EnvironmentMap = AssetManager::GetAsset<Texture>(AssetImporter::ImportTextureAsset(folder + std::string("/skybox.exr"), noCompressNoMipMapTexOptions));

		return true;
	}

	// Meshes
	if (className == "Sphere")
	{
		glm::vec3 center = glm::vec3(0, 0, 0);
		pb.GetProperty("O", center);

		float radius = 1.0f;
		pb.GetProperty("R", radius);

		MeshPtr mesh = AssetManager::GetAsset<Mesh>(AssetImporter::ImportMeshAsset("data/meshes/sphere.fbx", noFlipMeshOptions));
		m_Meshes[objectName] = mesh;
		m_MeshesTransforms[mesh].Translation = center; // not correct, but ok for now
		m_MeshesTransforms[mesh].Scale = { radius, radius, radius }; // not correct, but ok for now
		return true;
	}
	
	if (className == "Cube")
	{
		glm::vec3 center = glm::vec3(0, 0, 0);
		pb.GetProperty("O", center);

		float side = 1.0f;
		pb.GetProperty("side", side);

		MeshPtr mesh = AssetManager::GetAsset<Mesh>(AssetImporter::ImportMeshAsset("data/meshes/cube.obj", noFlipMeshOptions));
		m_Meshes[objectName] = mesh;
		m_MeshesTransforms[mesh].Translation = center;
		m_MeshesTransforms[mesh].Scale = { side, side, side };
		return true;
	}

	//if (className == "CSGUnion") return new CSGUnion;
	//if (className == "CSGInter") return new CSGInter;
	//if (className == "CSGDiff") return new CSGDiff;
	if (className == "Plane")
	{
		float y, limit;
		pb.GetProperty("y", y);
		pb.GetProperty("limit", limit);

		MeshPtr mesh = AssetManager::GetAsset<Mesh>(AssetImporter::ImportMeshAsset("data/meshes/cube.obj", noFlipMeshOptions));
		m_Meshes[objectName] = mesh;
		m_MeshesTransforms[mesh].Translation = { 0, y, 0 };
		m_MeshesTransforms[mesh].Scale = { limit, 1, limit };
		return true;
	}

	if (className == "Mesh")
	{
		char filename[256];
		if (!pb.GetFilenameProp("file", filename))
			pb.RequiredProp("file");

		m_Meshes[objectName] = AssetManager::GetAsset<Mesh>(AssetImporter::ImportMeshAsset(filename, noFlipMeshOptions));
		return true;
	}

	// Shaders
	if (className == "Lambert")
	{
		glm::vec4 color;
		pb.GetProperty("color", color);

		TexturePtr texture;
		pb.GetProperty("texture", texture);

		MaterialPtr material = std::make_shared<Material>(MaterialType::Lambert);
		material->SetProperty(MaterialPropertyType::AlbedoColor, color);
		material->SetTexture(MaterialTextureType::Albedo, texture);
		m_Materials[objectName] = material;
		return true;
	}

	if (className == "Phong")
	{
		glm::vec4 color;
		pb.GetProperty("color", color);

		glm::vec4 specular;
		pb.GetProperty("specular", specular);

		TexturePtr texture;
		pb.GetProperty("texture", texture);

		float exponent;
		pb.GetProperty("exponent", exponent);

		MaterialPtr material = std::make_shared<Material>(MaterialType::Phong);
		material->SetProperty(MaterialPropertyType::AlbedoColor, color);
		material->SetProperty(MaterialPropertyType::SpecularColor, specular);
		material->SetProperty(MaterialPropertyType::Shininess, exponent);
		material->SetTexture(MaterialTextureType::Albedo, texture);
		m_Materials[objectName] = material;
		return true;
	}

	//if (className == "Reflection") return new Reflection;
	//if (className == "Refraction") return new Refraction;
	//if (className == "Layered") return new Layered;
	//if (className == "Fresnel") return new Fresnel;

	// Lights
	if (className == "PointLight")
	{
		Entity e = m_Scene->CreateEntity(objectName);
		auto& light = e.AddComponent<PointLightComponent>();
		pb.GetProperty("color", light.Color);
		pb.GetProperty("power", light.Intensity);
		pb.GetProperty("pos", e.GetComponent<TransformComponent>().Translation);
		return true;
	}
	//if (className == "RectLight") return new RectLight;

	// Equivalent to Node is entity
	if (className == "Node")
	{
		MaterialPtr material;
		pb.GetProperty("shader", material);

		TexturePtr bump;
		pb.GetProperty("bump", bump);

		material->SetTexture(MaterialTextureType::Normal, bump);

		Entity node = m_Scene->CreateEntity(objectName);
		auto& mc = node.AddComponent<MeshComponent>();
		pb.GetProperty("geometry", mc.Mesh);
		pb.GetProperty(node.GetComponent<TransformComponent>());

		auto foundTransform = m_MeshesTransforms.find(mc.Mesh);
		if (foundTransform != m_MeshesTransforms.end())
		{
			auto& tc = node.GetComponent<TransformComponent>();
			tc.Translation += foundTransform->second.Translation;
			tc.Scale *= foundTransform->second.Scale;
		}

		mc.OverrideMaterialTable = std::make_shared<MaterialTable>(1);
		mc.OverrideMaterialTable->SetMaterial(0, material);
		return true;
	}

	return false;
}

bool DefaultSceneParser::ResolveFullPath(char* path)
{
	std::string temp = m_SceneRootDirectory;
	temp += path;
	if (std::filesystem::exists(temp)) {
		strcpy(path, temp.data());
		return true;
	}
	else {
		return false;
	}
}

static void StripWhiteSpace(char* s)
{
	int i = (int)strlen(s) - 1;
	while (i >= 0 && isspace(s[i])) i--;
	s[++i] = 0;
	int l = i;
	i = 0;
	while (i < l && isspace(s[i])) i++;
	if (i > 0 && i < l) {
		for (int j = 0; j <= l - i; j++)
			s[j] = s[i + j];
	}
}

bool DefaultSceneParser::Parse(const char* filename, Scene* ss, RendererDescription* rendererDesc)
{
	m_Scene = ss;
	m_RendererDescription = rendererDesc;
	m_CurrentLine = 0;

	//
	FILE* f = fopen(filename, "rt");
	if (!f)
	{
		fprintf(stderr, "Cannot open scene file `%s'!\n", filename);
		return false;
	}

	FileRAII fraii(f);
	char line[1024];
	bool commentedOut = false;
	std::vector<ParsedBlockImpl> parsedBlocks;
	ParsedBlockImpl* cblock = NULL;
	std::string currentClassName;
	std::string currentObjectName;
	while (fgets(line, sizeof(line), f))
	{
		m_CurrentLine++;
		if (commentedOut)
		{
			if (line[0] == '*' && line[1] == '/') commentedOut = false;
			continue;
		}
		char* commentBegin = NULL;
		if (strstr(line, "//")) commentBegin = strstr(line, "//");
		if (strstr(line, "#"))
		{
			char* temp = strstr(line, "#");
			if (!commentBegin || commentBegin > temp) commentBegin = temp;
		}
		if (commentBegin) *commentBegin = 0;
		StripWhiteSpace(line);
		if (strlen(line) == 0) continue; // empty line
		if (line[0] == '#' || (line[0] == '/' && line[1] == '/')) continue; // comment
		if (line[0] == '/' && line[1] == '*')
		{
			commentedOut = true;
			continue;
		}
		ReplaceRandomNumbers(m_CurrentLine, line);
		std::vector<std::string> tokens = Tokenize(line);
		if (!m_IsParsingObject)
		{
			switch (tokens.size())
			{
			case 1:
			{
				if (tokens[0] == "{")
					fprintf(stderr, "Excess `}' on line %d\n", m_CurrentLine);
				else
					fprintf(stderr, "Unexpected token `%s' on line %d\n", tokens[0].c_str(), m_CurrentLine);
				return false;
			}
			case 2:
			{
				if (tokens[1] != "{")
				{
					fprintf(stderr, "A singleton object definition should end with a `{' (on line %d)\n", m_CurrentLine);
					return false;
				}
				currentClassName = currentObjectName = tokens[0];
				m_IsParsingObject = true;
				break;
			}
			case 3:
			{
				if (tokens[2] != "{")
				{
					fprintf(stderr, "A object definition should end with a `{' (on line %d)\n", m_CurrentLine);
					return false;
				}
				currentClassName = tokens[0];
				currentObjectName = tokens[1];
				m_IsParsingObject = true;
				break;
			}
			default:
			{
				fprintf(stderr, "Unexpected content on line %d!\n", m_CurrentLine);
				return false;
			}
			}

			if (m_IsParsingObject)
			{
				parsedBlocks.push_back(ParsedBlockImpl());
				cblock = &parsedBlocks[parsedBlocks.size() - 1];
				cblock->parser = this;
				cblock->blockBegin = m_CurrentLine;
			}
			else
			{
				fprintf(stderr, "Unknown object class `%s' on line %d\n", tokens[0].c_str(), m_CurrentLine);
				return false;
			}
		}
		else
		{
			if (tokens.size() == 1)
			{
				if (tokens[0] == "}")
				{
					cblock->blockEnd = m_CurrentLine;

					try {
						AddSceneElement(currentClassName, currentObjectName, *cblock);
					}
					catch (SyntaxError err) {
						fprintf(stderr, "%s:%d: Syntax error on line %d: %s\n", filename, err.line, err.line, err.msg);
						return false;
					}
					catch (FileNotFoundError err) {
						fprintf(stderr, "%s:%d: Required file not found (%s) (required at line %d)\n", filename, err.line, err.filename, err.line);
						return false;
					}
					for (int i = 0; i < (int)cblock->m_Lines.size(); i++)
						if (!cblock->m_Lines[i].recognized)
							fprintf(stderr, "%s:%d: Warning: the property `%s' isn't recognized!\n", filename, cblock->m_Lines[i].line, cblock->m_Lines[i].propName);

					m_IsParsingObject = false;
					cblock = NULL;
					currentClassName = currentObjectName = "";
				}
				else
				{
					fprintf(stderr, "Unexpected token in object definition on line %d: `%s'\n", m_CurrentLine, tokens[0].c_str());
					return false;
				}
			}
			else
			{
				// try to find a property with that name...
				int i = (int)tokens[0].length();
				while (isspace(line[i])) i++;
				int l = (int)strlen(line) - 1;
				if (i < l && line[i] == '"' && line[l] == '"')
				{ // strip the quotes of a quoted argument
					line[l] = 0;
					i++;
				}
				ParsedBlockImpl::LineInfo info;
				cblock->m_Lines.push_back(ParsedBlockImpl::LineInfo(m_CurrentLine, tokens[0].c_str(), line + i));
			}
		}
	}

	return PostParse(filename, parsedBlocks);
}

MaterialPtr DefaultSceneParser::FindMaterialByName(const char* name) const
{
	auto found = m_Materials.find(name);
	return found != m_Materials.end() ? found->second : nullptr;
}

MeshPtr DefaultSceneParser::FindMeshByName(const char* name) const
{
	auto found = m_Meshes.find(name);
	return found != m_Meshes.end() ? found->second : nullptr;
}
TexturePtr DefaultSceneParser::FindTextureByName(const char* name) const
{
	auto found = m_Textures.find(name);
	return found != m_Textures.end() ? found->second : nullptr;
}

bool DefaultSceneParser::PostParse(const char* filename, std::vector<ParsedBlockImpl>& parsedBlocks)
{
	if (m_IsParsingObject)
	{
		fprintf(stderr, "Unfinished object definition at EOF!\n");
		return false;
	}

	return true;
}

void DefaultSceneParser::ReplaceRandomNumbers(int srcLine, char s[])
{
	static std::mt19937 generator; // mersenne twister generator
	while (strstr(s, "randfloat"))
	{
		char* p = strstr(s, "randfloat");
		int i, j;
		for (i = 0; p[i] && p[i] != '('; i++);
		if (p[i] != '(') throw SyntaxError(srcLine, "randfloat in inexpected format");
		for (j = i; p[j] && p[j] != ')'; j++);
		if (p[j] != ')') throw SyntaxError(srcLine, "randfloat in inexpected format");
		p[j] = 0;
		float f1, f2;
		if (2 != sscanf(p + i + 1, "%f,%f", &f1, &f2)) throw SyntaxError(srcLine, "bad randfloat format (expected: randfloat(<min>, <max>))");
		if (f1 > f2) throw SyntaxError(srcLine, "bad randfloat format (min > max)");
		std::uniform_real_distribution<float> sampler(f1, f2);
		float res = sampler(generator);
		for (int k = 0; k <= j; k++)
			p[k] = ' ';
		char temp[30];
		sprintf(temp, "%.5f", res);
		int l = (int)strlen(temp);
		assert(l < j);
		for (int i = 0; i < l; i++)
			p[i] = temp[i];
	}
	while (strstr(s, "randint"))
	{
		char* p = strstr(s, "randint");
		int i, j;
		for (i = 0; p[i] && p[i] != '('; i++);
		if (p[i] != '(') throw SyntaxError(srcLine, "randint in inexpected format");
		for (j = i; p[j] && p[j] != ')'; j++);
		if (p[j] != ')') throw SyntaxError(srcLine, "randint in inexpected format");
		p[j] = 0;
		int f1, f2;
		if (2 != sscanf(p + i + 1, "%d,%d", &f1, &f2)) throw SyntaxError(srcLine, "bad randint format (expected: randint(<min>, <max>))");
		if (f1 > f2) throw SyntaxError(srcLine, "bad randint format (min > max)");
		std::uniform_int_distribution<int> sampler(f1, f2);
		int res = sampler(generator);
		for (int k = 0; k <= j; k++)
			p[k] = ' ';
		char temp[30];
		sprintf(temp, "%d", res);
		int l = (int)strlen(temp);
		assert(l < j);
		for (int i = 0; i < l; i++)
			p[i] = temp[i];
	}
}
