#pragma once

#include "core/core.h"
#include "core/object.h"
#include "rendering/texture.h"

#include <glm.hpp>

enum class MaterialFlags
{
    None = 0,
    TwoSided = 1,
    Transparent = 2
};

ENUM_FLAGS(MaterialFlags);

enum class MaterialType
{
    Lambert,
    Phong,
    PBR
};

enum class MaterialPropertyType
{
    AlbedoColor,

    // Phong specific
    SpecularColor,
    Shininess,

    // PBR specific
    Roughness,
    Metalness
};

enum class MaterialTextureType
{
    Albedo,
    Normal,

    // PBR specific
    Roughness,
    Metalness
};

struct MaterialPropertyMetaData
{
    MaterialPropertyType Type;
    uint32_t Offset;
    uint32_t Size;
};

struct MaterialTextureMetaData
{
    MaterialTextureType Type;
    uint32_t Index;
};

class Material
{
public:
    Material(MaterialType type, MaterialFlags flags = MaterialFlags::None);

    void SetFlag(MaterialFlags flag, bool state);

    template<typename T>
    bool SetProperty(MaterialPropertyType propertyType, const T& value)
    {
        const MaterialPropertyMetaData* metaData = GetMaterialPropertyMetaData(propertyType);
        if (!metaData)
        {
            HEXRAY_ERROR("Material property with type {} does not exist for material type {}", (uint32_t)propertyType, (uint32_t)m_Type);
            return false;
        }

        if (metaData->Size != sizeof(T))
        {
            HEXRAY_ERROR("Size of property with type {} does not match the size of the value type!", (uint32_t)propertyType);
            return false;
        }

        if (value == *(T*)(&m_PropertiesBuffer[metaData->Offset]))
            return true;

        memcpy(m_PropertiesBuffer.data() + metaData->Offset, &value, metaData->Size);
        return true;
    }

    bool SetTexture(MaterialTextureType textureType, const TexturePtr& texture)
    {
        const MaterialTextureMetaData* metaData = GetMaterialTextureMetaData(textureType);
        if (!metaData)
        {
            HEXRAY_ERROR("Texture of type {} does not exist for material type {}", (uint32_t)textureType, (uint32_t)m_Type);
            return false;
        }

        m_Textures[metaData->Index] = texture;
        return true;
    }

    template<typename T>
    bool GetProperty(MaterialPropertyType propertyType, T& outData)
    {
        const MaterialPropertyMetaData* metaData = GetMaterialPropertyMetaData(propertyType);
        if (!metaData)
        {
            HEXRAY_ERROR("Material property with type {} does not exist for material type {}", (uint32_t)propertyType, (uint32_t)m_Type);
            return false;
        }

        if (metaData->Size != sizeof(T))
        {
            HEXRAY_ERROR("Size of property with type {} does not match the size of the value type!", (uint32_t)propertyType);
            return false;
        }

        outData = *(T*)(&m_PropertiesBuffer[metaData->Offset]);
        return true;
    }

    bool GetTexture(MaterialTextureType textureType, TexturePtr& outTexture) const
    {
        const MaterialTextureMetaData* metaData = GetMaterialTextureMetaData(textureType);
        if (!metaData)
        {
            HEXRAY_ERROR("Texture of type {} does not exist for material type {}", (uint32_t)textureType, (uint32_t)m_Type);
            return false;
        }

        outTexture = m_Textures[metaData->Index];
        return true;
    }

    inline bool HasProperty(MaterialPropertyType propertyType) const { return GetMaterialPropertyMetaData(propertyType) != nullptr; }
    inline bool HasTexture(MaterialTextureType textureType) const { return GetMaterialTextureMetaData(textureType) != nullptr; }

    inline MaterialType GetType() const { return m_Type; }
    inline bool GetFlag(MaterialFlags flag) const { return (m_Flags & flag) != MaterialFlags::None; }

    virtual void Serialize(ParsedBlock& pb);
private:
    const MaterialPropertyMetaData* GetMaterialPropertyMetaData(MaterialPropertyType propertyType) const;
    const MaterialTextureMetaData* GetMaterialTextureMetaData(MaterialTextureType textureType) const;
private:
    MaterialType m_Type;
    MaterialFlags m_Flags;
    std::vector<uint8_t> m_PropertiesBuffer;
    std::vector<TexturePtr> m_Textures;
private:
    static std::unordered_map<MaterialType, std::vector<MaterialPropertyMetaData>> ms_MaterialTypeProperties;
    static std::unordered_map<MaterialType, std::vector<MaterialTextureMetaData>> ms_MaterialTypeTextures;
};
