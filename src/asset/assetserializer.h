#pragma once

#include "core/core.h"
#include "asset/asset.h"
#include "rendering/texture.h"
#include "rendering/mesh.h"

class AssetSerializer
{
public:
    template<typename T>
    static bool Serialize(const std::filesystem::path& filepath, const std::shared_ptr<T>& asset);

    template<typename T>
    static bool Deserialize(const std::filesystem::path& filepath, std::shared_ptr<T>& outAsset);

    static bool DeserializeMetaData(const std::filesystem::path& filepath, AssetMetaData& assetMetaData);
private:
    static void SerializeMetaData(std::ofstream& stream, const AssetMetaData& metaData);
    static void DeserializeMetaData(std::ifstream& stream, AssetMetaData& metaData);
};