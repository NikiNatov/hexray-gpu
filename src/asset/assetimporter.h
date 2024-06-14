#pragma once

#include "core/core.h"
#include "asset/asset.h"
#include "rendering/texture.h"
#include "rendering/material.h"
#include "rendering/mesh.h"

struct aiMaterial;
struct aiScene;

class AssetImporter
{
public:
    static Uuid ImportTextureAsset(const std::filesystem::path& sourceFilepath);
    static Uuid ImportTextureAsset(const byte* compressedData, uint32_t dataSize, const std::string& assetName);
    static Uuid ImportMeshAsset(const std::filesystem::path& sourceFilepath);
    static Uuid ImportMaterialAsset(const aiMaterial* assimpMaterial, const aiScene* assimpScene, const std::filesystem::path& meshSourcePath);
    static Uuid CreateMeshAsset(const std::filesystem::path& filepath, const MeshDescription& meshDesc, const Vertex* vertexData, const uint32_t* indexData);
    static Uuid CreateMaterialAsset(const std::filesystem::path& filepath, MaterialType materialType, MaterialFlags materialFlags = MaterialFlags::None);
private:
    static bool ImportDDS(const std::filesystem::path& filepath, TextureDescription& outTextureDesc, std::vector<uint8_t>& outPixels);
    static bool ImportDDS(const uint8_t* data, uint32_t size, TextureDescription& outTextureDesc, std::vector<uint8_t>& outPixels);
    static bool ImportSTB(const std::filesystem::path& filepath, TextureDescription& outTextureDesc, std::vector<uint8_t>& outPixels);
    static bool ImportSTB(const uint8_t* data, uint32_t size, TextureDescription& outTextureDesc, std::vector<uint8_t>& outPixels);
};