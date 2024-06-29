#pragma once

#include "core/core.h"
#include "rendering/texture.h"
#include "asset/asset.h"
#include "rendering/shaders/resources.h"

#include <glm.hpp>

enum class MaterialFlags
{
    None = 0,
    TwoSided = 1,
    Transparent = 2
};

ENUM_FLAGS(MaterialFlags);

enum class MaterialPropertyType
{
    AlbedoColor,
    EmissiveColor,

    // Phong specific
    SpecularColor,
    Shininess,

    // PBR specific
    Roughness,
    Metalness,

    RefractionColor,
    IndexOfRefraction,

    ReflectionColor,

    Count,
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
    uint32_t Size;

    template <typename T>
    inline static MaterialPropertyMetaData Create(MaterialPropertyType type)
    {
        return { type, sizeof(T) };
    }
};

struct MaterialTextureMetaData
{
    MaterialTextureType Type;
    uint32_t Index;
};

class Material : public Asset
{
    friend class AssetSerializer;
    friend class DefaultSceneParser;
public:
    Material(MaterialType type, MaterialFlags flags = MaterialFlags::None);

    void SetFlag(MaterialFlags flag, bool state);

    template<typename T>
    bool SetProperty(MaterialPropertyType propertyType, const T& value)
    {
        uint32_t size, offset;
        if (!GetMaterialPropertyMetaData(propertyType, size, offset))
        {
            HEXRAY_ERROR("Material property with type {} does not exist for material type {}", (uint32_t)propertyType, (uint32_t)m_Type);
            return false;
        }

        if (size != sizeof(T))
        {
            HEXRAY_ERROR("Size of property with type {} does not match the size of the value type!", (uint32_t)propertyType);
            return false;
        }

        if (value == *(T*)(&m_PropertiesBuffer[offset]))
            return true;

        memcpy(m_PropertiesBuffer.data() + offset, &value, size);
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
    bool GetProperty(MaterialPropertyType propertyType, T& outData) const
    {
        uint32_t size, offset;
        if (!GetMaterialPropertyMetaData(propertyType, size, offset))
        {
            HEXRAY_ERROR("Material property with type {} does not exist for material type {}", (uint32_t)propertyType, (uint32_t)m_Type);
            return false;
        }

        if (size != sizeof(T))
        {
            HEXRAY_ERROR("Size of property with type {} does not match the size of the value type!", (uint32_t)propertyType);
            return false;
        }

        // Backwards compatibility
        if (m_PropertiesBuffer.size() <= offset)
        {
            memset(&outData, 0, sizeof(outData));
            return false;
        }

        outData = *(T*)(&m_PropertiesBuffer[offset]);
        return true;
    }

    bool TryCopyProperty(const MaterialPtr& source, MaterialPropertyType propertyType)
    {
        uint32_t size, offset;
        if (!source->GetMaterialPropertyMetaData(propertyType, size, offset))
        {
            return false;
        }

        memcpy(m_PropertiesBuffer.data() + offset, source->m_PropertiesBuffer.data() + offset, size);
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

    inline bool HasProperty(MaterialPropertyType propertyType) const
    {
        uint32_t dummySize, dummyOffset;
        return GetMaterialPropertyMetaData(propertyType, dummySize, dummyOffset);
    }

    inline bool HasTexture(MaterialTextureType textureType) const { return GetMaterialTextureMetaData(textureType) != nullptr; }

    inline MaterialType GetType() const { return m_Type; }
    inline bool GetFlag(MaterialFlags flag) const { return (m_Flags & flag) != MaterialFlags::None; }
private:
    bool GetMaterialPropertyMetaData(MaterialPropertyType propertyType, uint32_t& outSize, uint32_t& outOffset) const;
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

class MaterialTable
{
public:
    MaterialTable(uint32_t size)
    {
        Resize(size);
    }

    inline void Resize(uint32_t size) { m_Materials.resize(size); }
    inline void SetMaterial(uint32_t index, const MaterialPtr& material) { m_Materials[index] = material; }
    inline MaterialPtr GetMaterial(uint32_t index) const { return index < m_Materials.size() ? m_Materials[index] : nullptr; }
    inline bool IsValid(uint32_t index) const { return m_Materials[index] != nullptr; }

    inline uint32_t GetSize() const { return m_Materials.size(); }
    inline std::vector<MaterialPtr>::const_iterator begin() const { return m_Materials.begin(); }
    inline std::vector<MaterialPtr>::const_iterator end() const { return m_Materials.end(); }
private:
    std::vector<MaterialPtr> m_Materials;
};