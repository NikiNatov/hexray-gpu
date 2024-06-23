#pragma once

#include "core/core.h"
#include "asset/asset.h"
#include "rendering/texture.h"
#include "rendering/material.h"
#include "rendering/mesh.h"

struct aiMaterial;
struct aiScene;

struct TextureImportOptions
{
    bool Compress = true;
    bool GenerateMips = true;
};

struct MeshImportOptions
{
    bool ConvertToLeftHanded = true;
};

class AssetImporter
{
public:
    static Uuid ImportTextureAsset(const std::filesystem::path& sourceFilepath, TextureImportOptions options = TextureImportOptions());
    static Uuid ImportTextureAsset(const byte* compressedData, uint32_t dataSize, const std::string& assetName, TextureImportOptions options = TextureImportOptions());
    static Uuid ImportMeshAsset(const std::filesystem::path& sourceFilepath, MeshImportOptions options = MeshImportOptions());
    static Uuid ImportMaterialAsset(const aiMaterial* assimpMaterial, const aiScene* assimpScene, const std::filesystem::path& meshSourcePath);
    static Uuid CreateMeshAsset(const std::filesystem::path& filepath, const MeshDescription& meshDesc, const Vertex* vertexData, const uint32_t* indexData);
    static Uuid CreateMaterialAsset(const std::filesystem::path& filepath, MaterialType materialType, MaterialFlags materialFlags = MaterialFlags::None);
private:
    static bool GetExistingOrSetupImport(AssetType type, const std::string& assetName, const std::filesystem::path& sourcePath, AssetMetaData& outMetaData);
    static bool ImportDDS(const std::filesystem::path& filepath, TextureDescription& outTextureDesc, std::vector<uint8_t>& outPixels);
    static bool ImportDDS(const uint8_t* data, uint32_t size, TextureDescription& outTextureDesc, std::vector<uint8_t>& outPixels);
    static bool ImportSTB(const std::filesystem::path& filepath, TextureDescription& outTextureDesc, std::vector<uint8_t>& outPixels);
    static bool ImportSTB(const uint8_t* data, uint32_t size, TextureDescription& outTextureDesc, std::vector<uint8_t>& outPixels);
    static Uuid FinalizeTextureImport(TextureDescription& desc, std::vector<uint8_t>& pixels, const AssetMetaData& metaData, TextureImportOptions options);
    static bool GenerateMipmaps(TextureDescription& desc, std::vector<uint8_t>& pixels);
    static bool CompressDXT(TextureDescription& desc, std::vector<uint8_t>& pixels);
};