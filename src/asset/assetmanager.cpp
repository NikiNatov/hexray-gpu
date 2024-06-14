#include "assetmanager.h"

#include "asset/assetserializer.h"

std::filesystem::path AssetManager::ms_AssetsFolder;
std::unordered_map<Uuid, AssetMetaData> AssetManager::ms_AssetRegistry;
std::unordered_map<Uuid, std::shared_ptr<Asset>> AssetManager::ms_LoadedAssets;
std::unordered_map<std::filesystem::path, Uuid> AssetManager::ms_AssetPathUUIDs;

// ------------------------------------------------------------------------------------------------------------------------------------
void AssetManager::Initialize(const std::filesystem::path& assetFolder)
{
    Shutdown();

    if (!std::filesystem::exists(assetFolder))
        std::filesystem::create_directories(assetFolder);

    ms_AssetsFolder = std::filesystem::canonical(assetFolder);
    RegisterAllAssets(ms_AssetsFolder);
}

// ------------------------------------------------------------------------------------------------------------------------------------
void AssetManager::Shutdown()
{
    ms_AssetRegistry.clear();
    ms_AssetPathUUIDs.clear();
    ms_LoadedAssets.clear();
}

// ------------------------------------------------------------------------------------------------------------------------------------
void AssetManager::RegisterAsset(const AssetMetaData& metaData)
{
    if (!IsAssetFile(metaData.AssetFilepath))
    {
        HEXRAY_ERROR("Asset Manager: Failed registering asset {}. Unsupported asset file {}", metaData.AssetFilepath.extension().string());
        return;
    }

    if (!std::filesystem::exists(metaData.AssetFilepath))
    {
        HEXRAY_ERROR("Asset Manager: Failed registering asset {}. Asset path does not exist.", metaData.AssetFilepath.string());
        return;
    }

    ms_AssetRegistry[metaData.ID] = metaData;
    ms_AssetPathUUIDs[metaData.AssetFilepath] = metaData.ID;
}

// ------------------------------------------------------------------------------------------------------------------------------------
void AssetManager::RegisterAsset(const std::filesystem::path& assetPath)
{
    if (!IsAssetFile(assetPath))
        return;

    AssetMetaData metaData;
    if (!AssetSerializer::DeserializeMetaData(assetPath, metaData))
    {
        HEXRAY_ERROR("Asset Manager: Failed registering asset {}. Reading asset meta data failed.", assetPath.string());
        return;
    }

    ms_AssetRegistry[metaData.ID] = metaData;
    ms_AssetPathUUIDs[metaData.AssetFilepath] = metaData.ID;
}

// ------------------------------------------------------------------------------------------------------------------------------------
bool AssetManager::LoadAsset(Uuid id)
{
    if (!IsAssetValid(id))
    {
        HEXRAY_ERROR("Asset Manager: Failed loading asset with UUID = {}. Asset was not found in registry.", id);
        return false;
    }

    if (IsAssetLoaded(id))
        return true;

    const AssetMetaData& metaData = ms_AssetRegistry[id];
    
    bool result = false;

    if (metaData.Type == AssetType::Texture)
    {
        TexturePtr asset;
        if (!AssetSerializer::Deserialize(metaData.AssetFilepath, asset))
        {
            HEXRAY_ERROR("Asset Manager: Failed loading texture asset {}({}). Asset deserialization failed.", metaData.AssetFilepath.string(), id);
            return false;
        }

        ms_LoadedAssets[id] = asset;
    }
    else if (metaData.Type == AssetType::Mesh)
    {
        MeshPtr asset;
        if (!AssetSerializer::Deserialize(metaData.AssetFilepath, asset))
        {
            HEXRAY_ERROR("Asset Manager: Failed loading mesh asset {}({}). Asset deserialization failed.", metaData.AssetFilepath.string(), id);
            return false;
        }

        ms_LoadedAssets[id] = asset;
    }
    else if (metaData.Type == AssetType::Material)
    {
        MaterialPtr asset;
        if (!AssetSerializer::Deserialize(metaData.AssetFilepath, asset))
        {
            HEXRAY_ERROR("Asset Manager: Failed loading material asset {}({}). Asset deserialization failed.", metaData.AssetFilepath.string(), id);
            return false;
        }

        ms_LoadedAssets[id] = asset;
    }
    
    return true;
}

// ------------------------------------------------------------------------------------------------------------------------------------
Uuid AssetManager::GetUUIDForAssetPath(const std::filesystem::path& assetPath)
{
    auto it = ms_AssetPathUUIDs.find(assetPath);

    if (it == ms_AssetPathUUIDs.end())
        return 0;

    return it->second;
}

// ------------------------------------------------------------------------------------------------------------------------------------
bool AssetManager::IsAssetFile(const std::filesystem::path& filepath)
{
    for (uint32_t i = 0; i < (uint32_t)AssetType::NumTypes; i++)
    {
        if (filepath.extension() == Asset::AssetFileExtensions[i])
        {
            return true;
        }
    }

    return false;
}

// ------------------------------------------------------------------------------------------------------------------------------------
void AssetManager::RegisterAllAssets(const std::filesystem::path& assetFolder)
{
    HEXRAY_ASSERT(std::filesystem::exists(assetFolder));

    for (auto entry : std::filesystem::directory_iterator(assetFolder))
    {
        if (entry.is_directory())
        {
            RegisterAllAssets(entry);
        }
        else if (IsAssetFile(entry.path()))
        {
            RegisterAsset(entry);
        }
    }
}

// ------------------------------------------------------------------------------------------------------------------------------------
bool AssetManager::IsAssetValid(Uuid id)
{
    return ms_AssetRegistry.find(id) != ms_AssetRegistry.end();
}

// ------------------------------------------------------------------------------------------------------------------------------------
bool AssetManager::IsAssetLoaded(Uuid id)
{
    return ms_LoadedAssets.find(id) != ms_LoadedAssets.end();
}

// ------------------------------------------------------------------------------------------------------------------------------------
const AssetMetaData* AssetManager::GetAssetMetaData(Uuid id)
{
    auto it = ms_AssetRegistry.find(id);

    if (it != ms_AssetRegistry.end())
        return &it->second;

    return nullptr;
}
