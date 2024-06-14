#include "assetimporter.h"

#include "asset/assetmanager.h"
#include "asset/assetserializer.h"

#include <DirectXTex.h>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <fstream>

// ------------------------------------------------------------------------------------------------------------------------------------
Uuid AssetImporter::ImportTextureAsset(const std::filesystem::path& sourceFilepath)
{
    std::string assetFilename = sourceFilepath.stem().string() + Asset::AssetFileExtensions[(uint32_t)AssetType::Texture];
    std::filesystem::path destinationFolder = "textures";
    std::filesystem::path assetFullPath = AssetManager::GetAssetFullPath(destinationFolder / assetFilename);

    if (std::filesystem::exists(assetFullPath))
    {
        return AssetManager::GetUUIDForAssetPath(assetFullPath);
    }

    if (!std::filesystem::exists(assetFullPath.parent_path()))
    {
        std::filesystem::create_directories(assetFullPath.parent_path());
    }

    TextureDescription textureDesc;
    std::vector<uint8_t> pixels;

    if (sourceFilepath.extension() == ".dds")
    {
        if (!ImportDDS(sourceFilepath, textureDesc, pixels))
        {
            return Uuid::Invalid;
        }
    }
    else
    {
        if (!ImportSTB(sourceFilepath, textureDesc, pixels))
        {
            return Uuid::Invalid;
        }
    }

    TexturePtr texture = std::make_shared<Texture>(textureDesc, sourceFilepath.stem().wstring().c_str());
    texture->UploadGPUData(pixels.data(), true);

    texture->m_MetaData.SourceFilepath = sourceFilepath;

    if (!AssetSerializer::Serialize(assetFullPath, texture))
    {
        HEXRAY_ERROR("Asset Importer: Failed serializing texture asset {}", assetFullPath.string());
        return Uuid::Invalid;
    }

    AssetManager::RegisterAsset(texture->m_MetaData);
    return texture->m_MetaData.ID;
}

// ------------------------------------------------------------------------------------------------------------------------------------
Uuid AssetImporter::ImportTextureAsset(const byte* compressedData, uint32_t dataSize, const std::string& assetName)
{
    std::string assetFilename = assetName + Asset::AssetFileExtensions[(uint32_t)AssetType::Texture];
    std::filesystem::path destinationFolder = "textures";
    std::filesystem::path assetFullPath = AssetManager::GetAssetFullPath(destinationFolder / assetFilename);

    if (std::filesystem::exists(assetFullPath))
    {
        return AssetManager::GetUUIDForAssetPath(assetFullPath);
    }

    if (!std::filesystem::exists(assetFullPath.parent_path()))
    {
        std::filesystem::create_directories(assetFullPath.parent_path());
    }

    TextureDescription textureDesc;
    std::vector<uint8_t> pixels;

    if (!ImportSTB(compressedData, dataSize, textureDesc, pixels))
    {
        return Uuid::Invalid;
    }

    TexturePtr texture = std::make_shared<Texture>(textureDesc, assetFullPath.stem().wstring().c_str());
    texture->UploadGPUData(pixels.data(), true);

    if (!AssetSerializer::Serialize(assetFullPath, texture))
    {
        HEXRAY_ERROR("Asset Importer: Failed serializing texture asset {}", assetFullPath.string());
        return Uuid::Invalid;
    }

    AssetManager::RegisterAsset(texture->m_MetaData);
    return texture->m_MetaData.ID;
}

// ------------------------------------------------------------------------------------------------------------------------------------
Uuid AssetImporter::ImportMeshAsset(const std::filesystem::path& sourceFilepath)
{
    std::string assetFilename = sourceFilepath.stem().string() + Asset::AssetFileExtensions[(uint32_t)AssetType::Mesh];
    std::filesystem::path destinationFolder = "meshes";
    std::filesystem::path assetFullPath = AssetManager::GetAssetFullPath(destinationFolder / assetFilename);

    if (std::filesystem::exists(assetFullPath))
    {
        return AssetManager::GetUUIDForAssetPath(assetFullPath);
    }

    if (!std::filesystem::exists(assetFullPath.parent_path()))
    {
        std::filesystem::create_directories(assetFullPath.parent_path());
    }

    // Import asset data
    uint64_t processingFlags = aiProcess_ImproveCacheLocality |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_SplitLargeMeshes |
        aiProcess_Triangulate |
        aiProcess_GenUVCoords |
        aiProcess_CalcTangentSpace |
        aiProcess_SortByPType |
        aiProcess_FindDegenerates |
        aiProcess_FindInstances |
        aiProcess_ValidateDataStructure |
        aiProcess_OptimizeMeshes |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ConvertToLeftHanded |
        aiProcess_PreTransformVertices |
        aiProcess_GenSmoothNormals;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(sourceFilepath.string(), processingFlags);

    if (!scene)
    {
        HEXRAY_ERROR("Asset Importer: Failed decoding mesh file. Reason: {}", importer.GetErrorString());
        return Uuid::Invalid;
    }

    MeshDescription meshDesc;
    meshDesc.Submeshes.resize(scene->mNumMeshes);
    meshDesc.Materials.resize(scene->mNumMeshes);

    std::vector<Vertex> vertices;
    vertices.reserve(5000);

    std::vector<uint32_t> indices;
    indices.reserve(10000);

    // Parse all submeshes
    for (uint32_t submeshIdx = 0; submeshIdx < scene->mNumMeshes; submeshIdx++)
    {
        aiMesh* submesh = scene->mMeshes[submeshIdx];

        uint32_t startVertex = vertices.size();
        uint32_t startIndex = indices.size();

        // Construct all vertices
        uint32_t vertexCount = 0;
        for (uint32_t vertexIdx = 0; vertexIdx < submesh->mNumVertices; vertexIdx++)
        {
            const aiVector3D& position = submesh->mVertices[vertexIdx];
            const aiVector3D& texCoord = submesh->mTextureCoords[0][vertexIdx];
            const aiVector3D& normal = submesh->mNormals[vertexIdx];
            const aiVector3D& tangent = submesh->mTangents[vertexIdx];
            const aiVector3D& bitangent = submesh->mBitangents[vertexIdx];

            Vertex& v = vertices.emplace_back();
            v.Position = { position.x, position.y, position.z };
            v.TexCoord = { texCoord.x, texCoord.y };
            v.Normal = { normal.x, normal.y, normal.z };
            v.Tangent = { tangent.x, tangent.y, tangent.z };
            v.Bitangent = { bitangent.x, bitangent.y, bitangent.z };

            vertexCount++;
        }

        // Construct all indices
        uint32_t indexCount = 0;
        for (uint32_t faceIdx = 0; faceIdx < submesh->mNumFaces; faceIdx++)
        {
            const aiFace& face = submesh->mFaces[faceIdx];
            for (uint32_t i = 0; i < face.mNumIndices; i++)
            {
                indices.push_back(face.mIndices[i]);
                indexCount++;
            }
        }

        // Create submesh
        meshDesc.Submeshes[submeshIdx].StartVertex = startVertex;
        meshDesc.Submeshes[submeshIdx].VertexCount = vertexCount;
        meshDesc.Submeshes[submeshIdx].StartIndex = startIndex;
        meshDesc.Submeshes[submeshIdx].IndexCount = indexCount;
        meshDesc.Submeshes[submeshIdx].MaterialIndex = submesh->mMaterialIndex;
    }

    // Parse all materials
    for (uint32_t materialIdx = 0; materialIdx < scene->mNumMaterials; materialIdx++)
    {
        const aiMaterial* assimpMat = scene->mMaterials[materialIdx];

        Uuid materialID = ImportMaterialAsset(assimpMat, scene, sourceFilepath);
        if (materialID == Uuid::Invalid)
        {
            HEXRAY_ERROR("Asset Importer: Failed importing material from mesh source file {}", sourceFilepath.string());
            continue;
        }

        meshDesc.Materials[materialIdx] = AssetManager::GetAsset<Material>(materialID);
    }

    // Create and serialize asset
    MeshPtr mesh = std::make_shared<Mesh>(meshDesc, sourceFilepath.stem().wstring().c_str());
    mesh->UploadGPUData(vertices.data(), indices.data(), true);

    mesh->m_MetaData.SourceFilepath = sourceFilepath;

    if (!AssetSerializer::Serialize(assetFullPath, mesh))
    {
        HEXRAY_ERROR("Asset Importer: Failed serializing mesh asset {}", assetFullPath.string());
        return Uuid::Invalid;
    }

    AssetManager::RegisterAsset(mesh->m_MetaData);
    return mesh->m_MetaData.ID;
}

// ------------------------------------------------------------------------------------------------------------------------------------
Uuid AssetImporter::ImportMaterialAsset(const aiMaterial* assimpMaterial, const aiScene* assimpScene, const std::filesystem::path& meshSourcePath)
{
    aiString tmpName;
    assimpMaterial->Get(AI_MATKEY_NAME, tmpName);

    std::string materialName = tmpName.C_Str();
    materialName.append(Asset::AssetFileExtensions[(uint32_t)AssetType::Material]);
    std::replace(materialName.begin(), materialName.end(), ':', '_');

    std::filesystem::path destinationFolder = "materials";
    std::filesystem::path assetFullPath = AssetManager::GetAssetFullPath(destinationFolder / materialName);

    if (std::filesystem::exists(assetFullPath))
    {
        return AssetManager::GetUUIDForAssetPath(assetFullPath);
    }

    if (!std::filesystem::exists(assetFullPath.parent_path()))
    {
        std::filesystem::create_directories(assetFullPath.parent_path());
    }

    MaterialPtr material = std::make_shared<Material>(MaterialType::PBR);

    // Set albedo color
    aiColor4D albedo;
    if (assimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, albedo) == AI_SUCCESS)
    {
        // Set transparency flag
        float opacity;
        if (assimpMaterial->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS && opacity < 1.0f)
        {
            material->SetFlag(MaterialFlags::Transparent, true);
            albedo.a = opacity;
        }

        material->SetProperty(MaterialPropertyType::AlbedoColor, glm::vec4(albedo.r, albedo.g, albedo.b, albedo.a));
    }

    // Set roughness
    float roughness;
    if (assimpMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS)
    {
        material->SetProperty(MaterialPropertyType::Roughness, roughness);
    }

    // Set metalness
    float metalness;
    if (assimpMaterial->Get(AI_MATKEY_REFLECTIVITY, metalness) == AI_SUCCESS)
    {
        material->SetProperty(MaterialPropertyType::Metalness, metalness);
    }

    // Set two sided flag
    bool twoSided;
    if (assimpMaterial->Get(AI_MATKEY_TWOSIDED, twoSided) == AI_SUCCESS && twoSided)
    {
        material->SetFlag(MaterialFlags::TwoSided, true);
    }

    // Set textures
    auto SetMaterialTexture = [&](aiTextureType type, MaterialTextureType materialTextureType)
    {
        aiString aiPath;
        if (assimpMaterial->GetTexture(type, 0, &aiPath) == AI_SUCCESS)
        {
            TextureDescription textureDesc;
            textureDesc.Format = type == aiTextureType_METALNESS || type == aiTextureType_SHININESS ? DXGI_FORMAT_R8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM;

            Uuid textureUUID = Uuid::Invalid;
            if (const aiTexture* aiTexture = assimpScene->GetEmbeddedTexture(aiPath.C_Str()))
            {
                // Texture is embedded. Decode the data buffer.
                std::filesystem::path textureName = std::filesystem::path(aiTexture->mFilename.C_Str()).stem();
                textureUUID = ImportTextureAsset((byte*)aiTexture->pcData, aiTexture->mWidth, textureName.string());
            }
            else
            {
                // Load the texture from filepath
                textureUUID = ImportTextureAsset(meshSourcePath.parent_path() / aiPath.C_Str());
            }

            if (textureUUID == Uuid::Invalid)
            {
                HEXRAY_ERROR("Asset Importer: Failed to import texture from mesh file");
                return;
            }

            TexturePtr texture = AssetManager::GetAsset<Texture>(textureUUID);
            material->SetTexture(materialTextureType, texture);
        }
    };

    SetMaterialTexture(aiTextureType_DIFFUSE, MaterialTextureType::Albedo);
    SetMaterialTexture(aiTextureType_NORMALS, MaterialTextureType::Normal);
    SetMaterialTexture(aiTextureType_METALNESS, MaterialTextureType::Metalness);
    SetMaterialTexture(aiTextureType_SHININESS, MaterialTextureType::Roughness);

    // Serialize the material
    if (!AssetSerializer::Serialize(assetFullPath, material))
    {
        HEXRAY_ERROR("Asset Importer: Failed to serialize material {}", assetFullPath.string());
        return Uuid::Invalid;
    }

    AssetManager::RegisterAsset(material->m_MetaData);
    return material->GetMetaData().ID;
}

// ------------------------------------------------------------------------------------------------------------------------------------
Uuid AssetImporter::CreateMeshAsset(const std::filesystem::path& filepath, const MeshDescription& meshDesc, const Vertex* vertexData, const uint32_t* indexData)
{
    std::filesystem::path assetFullPath = AssetManager::GetAssetFullPath(filepath);

    if (std::filesystem::exists(filepath))
    {
        return AssetManager::GetUUIDForAssetPath(filepath);
    }

    if (!std::filesystem::exists(assetFullPath.parent_path()))
    {
        std::filesystem::create_directories(assetFullPath.parent_path());
    }

    MeshPtr asset = std::make_shared<Mesh>(meshDesc, filepath.stem().wstring().c_str());
    asset->UploadGPUData(vertexData, indexData, true);

    if (!AssetSerializer::Serialize(assetFullPath, asset))
    {
        HEXRAY_ERROR("Asset Importer: Failed to serialize mesh {}", assetFullPath.string());
        return Uuid::Invalid;
    }

    AssetManager::RegisterAsset(asset->m_MetaData);
    return asset->m_MetaData.ID;
}

// ------------------------------------------------------------------------------------------------------------------------------------
Uuid AssetImporter::CreateMaterialAsset(const std::filesystem::path& filepath, MaterialType materialType, MaterialFlags materialFlags)
{
    std::filesystem::path assetFullPath = AssetManager::GetAssetFullPath(filepath);

    if (std::filesystem::exists(filepath))
    {
        return AssetManager::GetUUIDForAssetPath(filepath);
    }

    if (!std::filesystem::exists(assetFullPath.parent_path()))
    {
        std::filesystem::create_directories(assetFullPath.parent_path());
    }

    MaterialPtr asset = std::make_shared<Material>(materialType, materialFlags);

    if (!AssetSerializer::Serialize(assetFullPath, asset))
    {
        HEXRAY_ERROR("Asset Importer: Failed to serialize material {}", assetFullPath.string());
        return Uuid::Invalid;
    }

    AssetManager::RegisterAsset(asset->m_MetaData);
    return asset->m_MetaData.ID;
}

// ------------------------------------------------------------------------------------------------------------------------------------
bool AssetImporter::ImportDDS(const std::filesystem::path& filepath, TextureDescription& outTextureDesc, std::vector<uint8_t>& outPixels)
{
    std::ifstream ifs(filepath, std::ios::in | std::ios::binary | std::ios::ate);

    if (!ifs)
    {
        HEXRAY_ERROR("Asset Importer: Could not open texture asset file: {}", filepath.string());
        return false;
    }

    uint32_t fileSize = ifs.tellg();
    std::vector<uint8_t> fileContents(fileSize);
    ifs.seekg(0, std::ios::beg);
    ifs.read((char*)fileContents.data(), fileSize);
    ifs.close();

    if (!ImportDDS(fileContents.data(), fileSize, outTextureDesc, outPixels))
    {
        HEXRAY_ERROR("Asset Importer: Failed decoding file: {}", filepath.string());
        return false;
    }

    return true;
}

// ------------------------------------------------------------------------------------------------------------------------------------
bool AssetImporter::ImportDDS(const uint8_t* data, uint32_t size, TextureDescription& outTextureDesc, std::vector<uint8_t>& outPixels)
{
    DirectX::ScratchImage imageData;
    HRESULT loadResult = DirectX::LoadFromDDSMemory(data, size, DirectX::DDS_FLAGS_NONE, nullptr, imageData);

    if (FAILED(loadResult))
    {
        HEXRAY_ERROR("Asset Importer: Failed decoding DDS data");
        return false;
    }

    const DirectX::TexMetadata& textureMetaData = imageData.GetMetadata();

    if (textureMetaData.dimension == DirectX::TEX_DIMENSION_TEXTURE1D || textureMetaData.dimension == DirectX::TEX_DIMENSION_TEXTURE3D)
    {
        HEXRAY_ERROR("Asset Importer: TEX_DIMENSION_TEXTURE1D and TEX_DIMENSION_TEXTURE3D are not currently supported");
        return false;
    }

    outTextureDesc.Format = textureMetaData.format;
    outTextureDesc.ArrayLevels = textureMetaData.arraySize;
    outTextureDesc.Width = textureMetaData.width;
    outTextureDesc.Height = textureMetaData.height;
    outTextureDesc.MipLevels = textureMetaData.mipLevels;
    outTextureDesc.IsCubeMap = textureMetaData.IsCubemap();

    outPixels.resize(imageData.GetPixelsSize());
    memcpy(outPixels.data(), imageData.GetPixels(), imageData.GetPixelsSize());

    return true;
}

// ------------------------------------------------------------------------------------------------------------------------------------
bool AssetImporter::ImportSTB(const std::filesystem::path& filepath, TextureDescription& outTextureDesc, std::vector<uint8_t>& outPixels)
{
    std::ifstream ifs(filepath, std::ios::in | std::ios::binary | std::ios::ate);

    if (!ifs)
    {
        HEXRAY_ERROR("Asset Importer: Could not open texture asset file: {}", filepath.string());
        return false;
    }

    uint32_t fileSize = ifs.tellg();
    std::vector<uint8_t> fileContents(fileSize);
    ifs.seekg(0, std::ios::beg);
    ifs.read((char*)fileContents.data(), fileSize);
    ifs.close();

    if (!ImportSTB(fileContents.data(), fileSize, outTextureDesc, outPixels))
    {
        HEXRAY_ERROR("Asset Importer: Failed decoding file: {}", filepath.string());
        return false;
    }

    return true;
}

// ------------------------------------------------------------------------------------------------------------------------------------
bool AssetImporter::ImportSTB(const uint8_t* data, uint32_t size, TextureDescription& outTextureDesc, std::vector<uint8_t>& outPixels)
{
    int32_t channels;
    int32_t width, height;
    uint8_t* pixels = nullptr;
    if (stbi_info_from_memory(data, int32_t(size), &width, &height, &channels))
    {
        if (stbi_is_hdr_from_memory(data, int32_t(size)))
        {
            switch (channels)
            {
                case 1: outTextureDesc.Format = DXGI_FORMAT_R32_FLOAT; break;
                case 2: outTextureDesc.Format = DXGI_FORMAT_R32G32_FLOAT; break;
                case 3: outTextureDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT; break;
                case 4: outTextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
            }

            pixels = (uint8_t*)stbi_loadf_from_memory(data, int32_t(size), &width, &height, &channels, channels);
        }
        else
        {
            if (channels == 3)
            {
                // Converts RGB to RGBA, since there are no RGB8 formats.
                channels = 4;
            }

            switch (channels)
            {
                case 1: outTextureDesc.Format = DXGI_FORMAT_R8_UNORM; break;
                case 2: outTextureDesc.Format = DXGI_FORMAT_R8G8_UNORM; break;
                case 4: outTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
            }

            pixels = stbi_load_from_memory(data, int32_t(size), &width, &height, &channels, channels);
        }
    }

    if (!pixels && stbi_failure_reason())
    {
        HEXRAY_ERROR("Asset Importer: Failed to decode image. Reason: {}", stbi_failure_reason());
        return false;
    }

    outTextureDesc.Width = width;
    outTextureDesc.Height = height;

    uint32_t textureSize = outTextureDesc.Width * outTextureDesc.Height * DirectX::BitsPerPixel(outTextureDesc.Format) / 8;
    outPixels.resize(textureSize);
    memcpy(outPixels.data(), pixels, textureSize);

    stbi_image_free(pixels);

    return true;
}