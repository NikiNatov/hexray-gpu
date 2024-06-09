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

class Material
{
public:
    Material(MaterialFlags flags = MaterialFlags::None);

    void SetFlag(MaterialFlags flag, bool state);
    inline void SetAlbedoColor(const glm::vec4& albedo) { m_AlbedoColor = albedo; }
    inline void SetRoughness(float roughness) { m_Roughness = roughness; }
    inline void SetMetalness(float metalness) { m_Metalness = metalness; }
    inline void SetAlbedoMap(const TexturePtr& albedoMap) { m_AlbedoMap = albedoMap; }
    inline void SetNormalMap(const TexturePtr& normalMap) { m_NormalMap = normalMap; }
    inline void SetRoughnessMap(const TexturePtr& roughnessMap) { m_RoughnessMap = roughnessMap; }
    inline void SetMetalnessMap(const TexturePtr& metalnessMap) { m_MetalnessMap = metalnessMap; }

    inline bool GetFlag(MaterialFlags flag) const { return (m_Flags & flag) != MaterialFlags::None; }
    inline const glm::vec4& GetAlbedoColor() const { return m_AlbedoColor; }
    inline float GetRoughness() const { return m_Roughness; }
    inline float GetMetalness() const { return m_Metalness; }
    inline const TexturePtr& GetAlbedoMap() const { return m_AlbedoMap; }
    inline const TexturePtr& GetNormalMap() const { return m_NormalMap; }
    inline const TexturePtr& GetRoughnessMap() const { return m_RoughnessMap; }
    inline const TexturePtr& GetMetalnessMap() const { return m_MetalnessMap; }

    virtual void Serialize(ParsedBlock& pb);
private:
    MaterialFlags m_Flags = MaterialFlags::None;
    glm::vec4 m_AlbedoColor = glm::vec4(1.0f);
    float m_Roughness = 0.5f;
    float m_Metalness = 0.5f;
    TexturePtr m_AlbedoMap;
    TexturePtr m_NormalMap;
    TexturePtr m_RoughnessMap;
    TexturePtr m_MetalnessMap;
};
