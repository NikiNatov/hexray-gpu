#include "meshloader.h"

#include "resourceloaders/textureloader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

// ------------------------------------------------------------------------------------------------------------------------------------
std::shared_ptr<Mesh> MeshLoader::LoadFromFile(const std::filesystem::path& filePath)
{
    std::string fileExtension = filePath.extension().string();
    return LoadAssimp(filePath);
}

// ------------------------------------------------------------------------------------------------------------------------------------
std::shared_ptr<Mesh> MeshLoader::LoadAssimp(const std::filesystem::path& filePath)
{
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
    const aiScene* scene = importer.ReadFile(filePath.string().c_str(), processingFlags);

    if (!scene)
    {
        HEXRAY_ERROR("Failed importing mesh file {}. Error: {}", filePath.string(), importer.GetErrorString());
        return nullptr;
    }

    MeshDescription meshDesc;
    meshDesc.Positions.reserve(5000);
    meshDesc.UVs.reserve(5000);
    meshDesc.Normals.reserve(5000);
    meshDesc.Tangents.reserve(5000);
    meshDesc.Bitangents.reserve(5000);
    meshDesc.Indices.reserve(10000);
    meshDesc.Submeshes.reserve(scene->mNumMeshes);
    meshDesc.Materials.reserve(scene->mNumMeshes);

    // Parse all submeshes
    for (uint32_t submeshIdx = 0; submeshIdx < scene->mNumMeshes; submeshIdx++)
    {
        aiMesh* submesh = scene->mMeshes[submeshIdx];

        uint32_t startVertex = meshDesc.Positions.size();
        uint32_t startIndex = meshDesc.Indices.size();

        // Construct all vertices
        uint32_t vertexCount = 0;
        for (uint32_t vertexIdx = 0; vertexIdx < submesh->mNumVertices; vertexIdx++)
        {
            const aiVector3D& position = submesh->mVertices[vertexIdx];
            const aiVector3D& texCoord = submesh->mTextureCoords[0][vertexIdx];
            const aiVector3D& normal = submesh->mNormals[vertexIdx];
            const aiVector3D& tangent = submesh->mTangents[vertexIdx];
            const aiVector3D& bitangent = submesh->mBitangents[vertexIdx];

            meshDesc.Positions.emplace_back(position.x, position.y, position.z);
            meshDesc.UVs.emplace_back(texCoord.x, texCoord.y);
            meshDesc.Normals.emplace_back(normal.x, normal.y, normal.z);
            meshDesc.Tangents.emplace_back(tangent.x, tangent.y, tangent.z);
            meshDesc.Bitangents.emplace_back(bitangent.x, bitangent.y, bitangent.z);
            vertexCount++;
        }

        // Construct all indices
        uint32_t indexCount = 0;
        for (uint32_t faceIdx = 0; faceIdx < submesh->mNumFaces; faceIdx++)
        {
            const aiFace& face = submesh->mFaces[faceIdx];
            for (uint32_t i = 0; i < face.mNumIndices; i++)
            {
                meshDesc.Indices.push_back(face.mIndices[i]);
                indexCount++;
            }
        }

        // Create submesh
        SubmeshDescription& sm = meshDesc.Submeshes.emplace_back();
        sm.StartVertex = startVertex;
        sm.VertexCount = vertexCount;
        sm.StartIndex = startIndex;
        sm.IndexCount = indexCount;
        sm.MaterialIndex = submesh->mMaterialIndex;
    }

    // Parse all materials
    for (uint32_t materialIdx = 0; materialIdx < scene->mNumMaterials; materialIdx++)
    {
        const aiMaterial* assimpMat = scene->mMaterials[materialIdx];

        std::shared_ptr<Material> material = std::make_shared<Material>(MaterialType::PBR);

        // Set albedo color
        aiColor4D albedo;
        if (assimpMat->Get(AI_MATKEY_COLOR_DIFFUSE, albedo) == AI_SUCCESS)
        {
            // Set transparency flag
            float opacity;
            if (assimpMat->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS && opacity < 1.0f)
            {
                material->SetFlag(MaterialFlags::Transparent, true);
                albedo.a = opacity;
            }

            material->SetProperty(MaterialPropertyType::AlbedoColor, glm::vec4(albedo.r, albedo.g, albedo.b, albedo.a));
        }

        // Set roughness
        float roughness;
        if (assimpMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS)
        {
            material->SetProperty(MaterialPropertyType::Roughness, roughness);
        }

        // Set metalness
        float metalness;
        if (assimpMat->Get(AI_MATKEY_REFLECTIVITY, metalness) == AI_SUCCESS)
        {
            material->SetProperty(MaterialPropertyType::Metalness, metalness);
        }

        // Set two sided flag
        bool twoSided;
        if (assimpMat->Get(AI_MATKEY_TWOSIDED, twoSided) == AI_SUCCESS && twoSided)
        {
            material->SetFlag(MaterialFlags::TwoSided, true);
        }

        // Set textures
        auto SetMaterialTexture = [&](aiTextureType type, MaterialTextureType materialTextureType)
        {
            aiString aiPath;
            if (assimpMat->GetTexture(type, 0, &aiPath) == AI_SUCCESS)
            {
                TextureDescription textureDesc;
                textureDesc.Format = type == aiTextureType_METALNESS || type == aiTextureType_SHININESS ? DXGI_FORMAT_R8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM;

                std::shared_ptr<Texture> texture;
                if (const aiTexture* aiTexture = scene->GetEmbeddedTexture(aiPath.C_Str()))
                {
                    // Texture is embedded. Decode the data buffer.
                    texture = TextureLoader::LoadFromCompressedData((uint8_t*)aiTexture->pcData, aiTexture->mWidth);
                }
                else
                {
                    // Load the texture from filepath
                    std::filesystem::path textureFullpath = filePath.parent_path() / aiPath.C_Str();
                    texture = TextureLoader::LoadFromFile(textureFullpath.string());
                }

                material->SetTexture(materialTextureType, texture);
            }
        };

        SetMaterialTexture(aiTextureType_DIFFUSE, MaterialTextureType::Albedo);
        SetMaterialTexture(aiTextureType_NORMALS, MaterialTextureType::Normal);
        SetMaterialTexture(aiTextureType_METALNESS, MaterialTextureType::Metalness);
        SetMaterialTexture(aiTextureType_SHININESS, MaterialTextureType::Roughness);

        meshDesc.Materials.push_back(material);
    }

    std::wstring meshName = filePath.stem().wstring();
    return std::make_shared<Mesh>(meshDesc, meshName.c_str());
}
