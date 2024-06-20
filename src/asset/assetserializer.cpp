#include "assetserializer.h"

#include "asset/assetmanager.h"
#include <fstream>

// ------------------------------------------------------------------------------------------------------------------------------------
template<>
static bool AssetSerializer::Serialize(const std::filesystem::path& filepath, const std::shared_ptr<Texture>& asset)
{
    HEXRAY_ASSERT(!asset->GetPixels().empty());

    std::ofstream ofs(filepath, std::ios::out | std::ios::binary | std::ios::trunc);

    if (!ofs)
    {
        HEXRAY_ERROR("Asset Serializer: Failed creating/opening texture file {}", filepath.string());
        return false;
    }

    std::filesystem::path absolutePath = std::filesystem::canonical(filepath);

    if (asset->GetAssetFlag(AssetFlags::Serialized) && asset->GetMetaData().AssetFilepath != absolutePath)
    {
        // If the asset was already serialized but the path is different than the one passed as a paraeter, create a copy of the asset with a new ID
        AssetMetaData newMetaData = asset->GetMetaData();
        newMetaData.ID = Uuid();
        newMetaData.AssetFilepath = absolutePath;
        SerializeMetaData(ofs, newMetaData);
    }
    else
    {
        asset->m_MetaData.AssetFilepath = absolutePath;
        asset->SetAssetFlag(AssetFlags::Serialized);
        SerializeMetaData(ofs, asset->m_MetaData);
    }

    DXGI_FORMAT format = asset->GetFormat();
    ofs.write((char*)&format, sizeof(format));

    uint32_t width = asset->GetWidth();
    ofs.write((char*)&width, sizeof(width));
    
    uint32_t height = asset->GetHeight();
    ofs.write((char*)&height, sizeof(height));

    uint32_t mipLevels = asset->GetMipLevels();
    ofs.write((char*)&mipLevels, sizeof(mipLevels));

    uint32_t arraySize = asset->GetArrayLevels();
    ofs.write((char*)&arraySize, sizeof(arraySize));

    bool isCubeMap = asset->IsCubeMap();
    ofs.write((char*)&isCubeMap, sizeof(isCubeMap));

    uint32_t pixelsSize = asset->GetPixels().size();
    ofs.write((char*)&pixelsSize, sizeof(pixelsSize));

    const std::vector<uint8_t>& pixels = asset->GetPixels();
    ofs.write((char*)pixels.data(), pixelsSize);

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------
template<>
static bool AssetSerializer::Serialize(const std::filesystem::path& filepath, const std::shared_ptr<Material>& asset)
{
    std::ofstream ofs(filepath, std::ios::out | std::ios::binary | std::ios::trunc);

    if (!ofs)
    {
        HEXRAY_ERROR("Asset Serializer: Failed creating/opening material file {}", filepath.string());
        return false;
    }

    std::filesystem::path absolutePath = std::filesystem::canonical(filepath);

    if (asset->GetAssetFlag(AssetFlags::Serialized) && asset->m_MetaData.AssetFilepath != absolutePath)
    {
        // If the asset was already serialized but the path is different than the one passed as a paraeter, create a copy of the asset with a new ID
        AssetMetaData newMetaData = asset->m_MetaData;
        newMetaData.ID = Uuid();
        newMetaData.AssetFilepath = absolutePath;
        SerializeMetaData(ofs, newMetaData);
    }
    else
    {
        asset->m_MetaData.AssetFilepath = absolutePath;
        asset->SetAssetFlag(AssetFlags::Serialized);
        SerializeMetaData(ofs, asset->m_MetaData);
    }

    MaterialType type = asset->GetType();
    ofs.write((char*)&type, sizeof(type));

    MaterialFlags flags = asset->m_Flags;
    ofs.write((char*)&flags, sizeof(MaterialFlags));

    uint32_t propertiesDataSize = asset->m_PropertiesBuffer.size();
    ofs.write((char*)&propertiesDataSize, sizeof(propertiesDataSize));

    const std::vector<uint8_t>& propertiesData = asset->m_PropertiesBuffer;
    ofs.write((char*)propertiesData.data(), propertiesDataSize);

    uint32_t textureCount = asset->m_Textures.size();
    ofs.write((char*)&textureCount, sizeof(textureCount));

    for (const TexturePtr& texture : asset->m_Textures)
    {
        Uuid texutreID = texture ? texture->GetID() : Uuid::Invalid;
        ofs.write((char*)&texutreID, sizeof(texutreID));
    }

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------
template<>
static bool AssetSerializer::Serialize(const std::filesystem::path& filepath, const std::shared_ptr<Mesh>& asset)
{
    HEXRAY_ASSERT(!asset->GetVertices().empty() && !asset->GetIndices().empty());

    std::ofstream ofs(filepath, std::ios::out | std::ios::binary | std::ios::trunc);

    if (!ofs)
    {
        HEXRAY_ERROR("Asset Serializer: Failed creating/opening mesh file {}", filepath.string());
        return false;
    }

    std::filesystem::path absolutePath = std::filesystem::canonical(filepath);

    if (asset->GetAssetFlag(AssetFlags::Serialized) && asset->m_MetaData.AssetFilepath != absolutePath)
    {
        // If the asset was already serialized but the path is different than the one passed as a paraeter, create a copy of the asset with a new ID
        AssetMetaData newMetaData = asset->m_MetaData;
        newMetaData.ID = Uuid();
        newMetaData.AssetFilepath = absolutePath;
        SerializeMetaData(ofs, newMetaData);
    }
    else
    {
        asset->m_MetaData.AssetFilepath = absolutePath;
        asset->SetAssetFlag(AssetFlags::Serialized);
        SerializeMetaData(ofs, asset->m_MetaData);
    }

    uint32_t vertexCount = asset->GetVertices().size();
    ofs.write((char*)&vertexCount, sizeof(vertexCount));

    const std::vector<Vertex>& vertexData = asset->GetVertices();
    ofs.write((char*)vertexData.data(), vertexCount * sizeof(Vertex));

    uint32_t indexCount = asset->GetIndices().size();
    ofs.write((char*)&indexCount, sizeof(indexCount));

    const std::vector<uint32_t>& indexData = asset->GetIndices();
    ofs.write((char*)indexData.data(), indexCount * sizeof(uint32_t));

    uint32_t submeshCount = asset->GetSubmeshes().size();
    ofs.write((char*)&submeshCount, sizeof(submeshCount));

    const std::vector<Submesh>& submeshes = asset->GetSubmeshes();
    ofs.write((char*)submeshes.data(), sizeof(Submesh) * submeshCount);

    uint32_t materialCount = asset->GetMaterialTable()->GetSize();
    ofs.write((char*)&materialCount, sizeof(materialCount));

    std::shared_ptr<MaterialTable> materials = asset->GetMaterialTable();

    for (const MaterialPtr& material : *materials)
    {
        Uuid materialID = material ? material->GetID() : Uuid::Invalid;
        ofs.write((char*)&materialID, sizeof(materialID));
    }

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------
template<>
bool AssetSerializer::Deserialize(const std::filesystem::path& filepath, std::shared_ptr<Texture>& outAsset)
{
    std::ifstream ifs(filepath, std::ios::in | std::ios::binary);

    if (!ifs)
    {
        HEXRAY_ERROR("Asset Serializer: Failed reading texture file {}", filepath.string());
        return false;
    }

    AssetMetaData metaData;
    metaData.AssetFilepath = std::filesystem::canonical(filepath);
    DeserializeMetaData(ifs, metaData);
    HEXRAY_ASSERT(metaData.Type == AssetType::Texture);

    TextureDescription textureDesc;

    ifs.read((char*)&textureDesc.Format, sizeof(textureDesc.Format));
    ifs.read((char*)&textureDesc.Width, sizeof(textureDesc.Width));
    ifs.read((char*)&textureDesc.Height, sizeof(textureDesc.Height));
    ifs.read((char*)&textureDesc.MipLevels, sizeof(textureDesc.MipLevels));
    ifs.read((char*)&textureDesc.ArrayLevels, sizeof(textureDesc.ArrayLevels));
    ifs.read((char*)&textureDesc.IsCubeMap, sizeof(textureDesc.IsCubeMap));

    uint32_t pixelsSize;
    ifs.read((char*)&pixelsSize, sizeof(pixelsSize));

    std::vector<uint8_t> pixels(pixelsSize);
    ifs.read((char*)pixels.data(), pixelsSize);

    outAsset = std::make_shared<Texture>(textureDesc, filepath.stem().wstring().c_str());
    outAsset->UploadGPUData(pixels.data());
    outAsset->m_MetaData = metaData;

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------
template<>
bool AssetSerializer::Deserialize(const std::filesystem::path& filepath, std::shared_ptr<Material>& outAsset)
{
    std::ifstream ifs(filepath, std::ios::in | std::ios::binary);

    if (!ifs)
    {
        HEXRAY_ERROR("Asset Serializer: Failed reading material file {}", filepath.string());
        return false;
    }

    AssetMetaData metaData;
    metaData.AssetFilepath = std::filesystem::canonical(filepath);
    DeserializeMetaData(ifs, metaData);
    HEXRAY_ASSERT(metaData.Type == AssetType::Material);

    MaterialType type;
    ifs.read((char*)&type, sizeof(type));

    MaterialFlags flags;
    ifs.read((char*)&flags, sizeof(MaterialFlags));

    outAsset = std::make_shared<Material>(type, flags);
    outAsset->m_MetaData = metaData;

    uint32_t propertiesDataSize;
    ifs.read((char*)&propertiesDataSize, sizeof(propertiesDataSize));

    outAsset->m_PropertiesBuffer.resize(propertiesDataSize);
    ifs.read((char*)outAsset->m_PropertiesBuffer.data(), propertiesDataSize);

    uint32_t textureCount;
    ifs.read((char*)&textureCount, sizeof(textureCount));

    outAsset->m_Textures.resize(textureCount, nullptr);
    for (uint32_t i = 0; i < textureCount; i++)
    {
        Uuid textureID;
        ifs.read((char*)&textureID, sizeof(textureID));

        if (textureID != Uuid::Invalid)
        {
            TexturePtr texture = AssetManager::GetAsset<Texture>(textureID);
            outAsset->m_Textures[i] = texture;
        }
    }

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------
template<>
bool AssetSerializer::Deserialize(const std::filesystem::path& filepath, std::shared_ptr<Mesh>& outAsset)
{
    std::ifstream ifs(filepath, std::ios::in | std::ios::binary);

    if (!ifs)
    {
        HEXRAY_ERROR("Asset Serializer: Failed reading file {}", filepath.string());
        return false;
    }

    AssetMetaData metaData;
    metaData.AssetFilepath = std::filesystem::canonical(filepath);
    DeserializeMetaData(ifs, metaData);
    HEXRAY_ASSERT(metaData.Type == AssetType::Mesh);

    MeshDescription meshDesc;

    uint32_t vertexCount;
    ifs.read((char*)&vertexCount, sizeof(vertexCount));

    std::vector<Vertex> vertexData(vertexCount);
    ifs.read((char*)vertexData.data(), vertexCount * sizeof(Vertex));

    uint32_t indexCount;
    ifs.read((char*)&indexCount, sizeof(indexCount));

    std::vector<uint32_t> indexData(indexCount);
    ifs.read((char*)indexData.data(), indexCount * sizeof(uint32_t));

    uint32_t submeshCount;
    ifs.read((char*)&submeshCount, sizeof(submeshCount));

    meshDesc.Submeshes.resize(submeshCount);
    ifs.read((char*)meshDesc.Submeshes.data(), sizeof(Submesh) * submeshCount);

    uint32_t materialCount;
    ifs.read((char*)&materialCount, sizeof(materialCount));

    meshDesc.MaterialTable = std::make_shared<MaterialTable>(materialCount);
    for (uint32_t i = 0; i < materialCount; i++)
    {
        Uuid materialID;
        ifs.read((char*)&materialID, sizeof(materialID));

        if (materialID != Uuid::Invalid)
        {
            MaterialPtr material = AssetManager::GetAsset<Material>(materialID);
            meshDesc.MaterialTable->SetMaterial(i, material);
        }
    }

    outAsset = std::make_shared<Mesh>(meshDesc, filepath.stem().wstring().c_str());
    outAsset->UploadGPUData(vertexData.data(), indexData.data());
    outAsset->m_MetaData = metaData;

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------
bool AssetSerializer::DeserializeMetaData(const std::filesystem::path& filepath, AssetMetaData& assetMetaData)
{
    std::ifstream ifs(filepath, std::ios::in | std::ios::binary);

    if (!ifs)
    {
        HEXRAY_ERROR("Asset Serializer: Failed reading file {}", filepath.string());
        return false;
    }

    DeserializeMetaData(ifs, assetMetaData);
    assetMetaData.AssetFilepath = std::filesystem::canonical(filepath);

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------
void AssetSerializer::SerializeMetaData(std::ofstream& stream, const AssetMetaData& metaData)
{
    std::string sourcePath = metaData.SourceFilepath.string();
    uint32_t sourcePathSize = sourcePath.size();
    stream.write((char*)&metaData.ID, sizeof(metaData.ID));
    stream.write((char*)&metaData.Type, sizeof(metaData.Type));
    stream.write((char*)&metaData.Flags, sizeof(metaData.Flags));
    stream.write((char*)&sourcePathSize, sizeof(sourcePathSize));
    stream.write(sourcePath.data(), sourcePathSize);
}

// -----------------------------------------------------------------------------------------------------------------------------
void AssetSerializer::DeserializeMetaData(std::ifstream& stream, AssetMetaData& metaData)
{
    stream.read((char*)&metaData.ID, sizeof(metaData.ID));
    stream.read((char*)&metaData.Type, sizeof(metaData.Type));
    stream.read((char*)&metaData.Flags, sizeof(metaData.Flags));

    uint32_t sourcePathSize;
    stream.read((char*)&sourcePathSize, sizeof(sourcePathSize));
    
    std::string sourcePath(sourcePathSize, '\0');
    stream.read(sourcePath.data(), sourcePathSize);

    metaData.SourceFilepath = sourcePath;
}

