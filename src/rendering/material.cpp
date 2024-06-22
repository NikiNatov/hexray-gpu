#include "material.h"

std::unordered_map<MaterialType, std::vector<MaterialPropertyMetaData>> Material::ms_MaterialTypeProperties = {
    { 
        MaterialType::Lambert,
        {
            MaterialPropertyMetaData{ MaterialPropertyType::AlbedoColor, 0, 16 }, // type, offset, size
            MaterialPropertyMetaData{ MaterialPropertyType::EmissiveColor, 16, 16 },
        },
    },

    {
        MaterialType::Phong,
        {
            MaterialPropertyMetaData{ MaterialPropertyType::AlbedoColor, 0, 16 },
            MaterialPropertyMetaData{ MaterialPropertyType::EmissiveColor, 16, 16 },
            MaterialPropertyMetaData{ MaterialPropertyType::SpecularColor, 32, 16 },
            MaterialPropertyMetaData{ MaterialPropertyType::Shininess, 48, 4 },
        },
    },

    {
        MaterialType::PBR,
        {
            MaterialPropertyMetaData{ MaterialPropertyType::AlbedoColor, 0, 16 },
            MaterialPropertyMetaData{ MaterialPropertyType::EmissiveColor, 16, 16 },
            MaterialPropertyMetaData{ MaterialPropertyType::Roughness, 32, 4 },
            MaterialPropertyMetaData{ MaterialPropertyType::Metalness, 36, 4 },
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
const MaterialPropertyMetaData* Material::GetMaterialPropertyMetaData(MaterialPropertyType propertyType) const
{
    for (const MaterialPropertyMetaData& metaData : ms_MaterialTypeProperties.at(m_Type))
    {
        if (metaData.Type == propertyType)
        {
            return &metaData;
        }
    }

    return nullptr;
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