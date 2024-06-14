#pragma once

#include "core/core.h"
#include "asset/asset.h"

class AssetManager
{
public:
    static void Initialize(const std::filesystem::path& assetFolder);
    static void Shutdown();
    static void RegisterAsset(const AssetMetaData& metaData);
    static void RegisterAsset(const std::filesystem::path& assetPath);
    static bool LoadAsset(Uuid id);
    static bool IsAssetValid(Uuid id);
    static bool IsAssetLoaded(Uuid id);
    static const AssetMetaData* GetAssetMetaData(Uuid id);
    static Uuid GetUUIDForAssetPath(const std::filesystem::path& assetPath);
    static bool IsAssetFile(const std::filesystem::path& filepath);

    template<typename T>
    static std::shared_ptr<T> GetAsset(Uuid id)
    {
        if (!LoadAsset(id))
            return nullptr;

        return std::dynamic_pointer_cast<T>(ms_LoadedAssets[id]);
    }

    inline static std::filesystem::path GetAssetFullPath(const std::filesystem::path& assetPath) { return ms_AssetsFolder / assetPath; }
    inline static const std::filesystem::path& GetAssetsFolder() { return ms_AssetsFolder; }
    inline static const std::unordered_map<Uuid, AssetMetaData>& GetAssetRegistry() { return ms_AssetRegistry; }
private:
    static void RegisterAllAssets(const std::filesystem::path& assetFolder);
private:
    static std::filesystem::path ms_AssetsFolder;
    static std::unordered_map<Uuid, AssetMetaData> ms_AssetRegistry;
    static std::unordered_map<Uuid, std::shared_ptr<Asset>> ms_LoadedAssets;
    static std::unordered_map<std::filesystem::path, Uuid> ms_AssetPathUUIDs;
};