#include "material.h"

std::unordered_map<MaterialType, std::vector<MaterialPropertyMetaData>> Material::ms_MaterialTypeProperties = {
    { 
        MaterialType::Lambert,
        {
            MaterialPropertyMetaData::Create<glm::vec4>(MaterialPropertyType::AlbedoColor),
            MaterialPropertyMetaData::Create<glm::vec4>(MaterialPropertyType::EmissiveColor),
            MaterialPropertyMetaData::Create<glm::vec3>(MaterialPropertyType::RefractionColor),
            MaterialPropertyMetaData::Create<float>(MaterialPropertyType::IndexOfRefraction),
            MaterialPropertyMetaData::Create<glm::vec3>(MaterialPropertyType::ReflectionColor),
        },
    },

    {
        MaterialType::Phong,
        {
            MaterialPropertyMetaData::Create<glm::vec4>(MaterialPropertyType::AlbedoColor),
            MaterialPropertyMetaData::Create<glm::vec4>(MaterialPropertyType::EmissiveColor),
            MaterialPropertyMetaData::Create<glm::vec4>(MaterialPropertyType::SpecularColor),
            MaterialPropertyMetaData::Create<float>(MaterialPropertyType::Shininess),
            MaterialPropertyMetaData::Create<glm::vec3>(MaterialPropertyType::RefractionColor),
            MaterialPropertyMetaData::Create<float>(MaterialPropertyType::IndexOfRefraction),
            MaterialPropertyMetaData::Create<glm::vec3>(MaterialPropertyType::ReflectionColor),
        },
    },

    {
        MaterialType::PBR,
        {
            MaterialPropertyMetaData::Create<glm::vec4>(MaterialPropertyType::AlbedoColor),
            MaterialPropertyMetaData::Create<glm::vec4>(MaterialPropertyType::EmissiveColor),
            MaterialPropertyMetaData::Create<float>(MaterialPropertyType::Roughness),
            MaterialPropertyMetaData::Create<float>(MaterialPropertyType::Metalness),
            MaterialPropertyMetaData::Create<glm::vec3>(MaterialPropertyType::RefractionColor),
            MaterialPropertyMetaData::Create<float>(MaterialPropertyType::IndexOfRefraction),
            MaterialPropertyMetaData::Create<glm::vec3>(MaterialPropertyType::ReflectionColor),
        },
    },
};

std::unordered_map<MaterialType, std::vector<MaterialTextureMetaData>> Material::ms_MaterialTypeTextures = {
    {
        MaterialType::Lambert,
        {
            MaterialTextureMetaData{ MaterialTextureType::Albedo, 0 }, // type, index
            MaterialTextureMetaData{ MaterialTextureType::Normal, 1 }
        },
    },

    {
        MaterialType::Phong,
        {
            MaterialTextureMetaData{ MaterialTextureType::Albedo, 0 },
            MaterialTextureMetaData{ MaterialTextureType::Normal, 1 }
        },
    },

    {
        MaterialType::PBR,
        {
            MaterialTextureMetaData{ MaterialTextureType::Albedo, 0 },
            MaterialTextureMetaData{ MaterialTextureType::Normal, 1 },
            MaterialTextureMetaData{ MaterialTextureType::Roughness, 2 },
            MaterialTextureMetaData{ MaterialTextureType::Metalness, 3 },
        },
    },
};

// ------------------------------------------------------------------------------------------------------------------------------------
Material::Material(MaterialType type,MaterialFlags flags)
    : Asset(AssetType::Material), m_Type(type), m_Flags(flags)
{
    uint32_t propertiesBufferSize = 0;
    for (const MaterialPropertyMetaData& metaData : ms_MaterialTypeProperties.at(m_Type))
    {
        propertiesBufferSize += metaData.Size;
    }

    m_PropertiesBuffer.resize(propertiesBufferSize);
    memset(m_PropertiesBuffer.data(), 0, propertiesBufferSize);

    uint32_t textureCount = ms_MaterialTypeTextures.at(m_Type).size();
    m_Textures.resize(textureCount);
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Material::SetFlag(MaterialFlags flag, bool state)
{
    if (state)
        m_Flags |= flag;
    else
        m_Flags &= ~flag;
}

// ------------------------------------------------------------------------------------------------------------------------------------
bool Material::GetMaterialPropertyMetaData(MaterialPropertyType propertyType, uint32_t& outSize, uint32_t& outOffset) const
{
    uint32_t offset = 0;
    for (const MaterialPropertyMetaData& metaData : ms_MaterialTypeProperties.at(m_Type))
    {
        if (metaData.Type == propertyType)
        {
            outSize = metaData.Size;
            outOffset = offset;
            return true;
        }

        offset += metaData.Size;
    }

    return false;
}

// ------------------------------------------------------------------------------------------------------------------------------------
const MaterialTextureMetaData* Material::GetMaterialTextureMetaData(MaterialTextureType textureType) const
{
    for (const MaterialTextureMetaData& metaData : ms_MaterialTypeTextures.at(m_Type))
    {
        if (metaData.Type == textureType)
        {
            return &metaData;
        }
    }

    return nullptr;
}