#pragma once

#include "core/core.h"
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
    inline void SetAlbedoColor(const glm::vec3& albedo) { m_AlbedoColor = albedo; }
    inline void SetRoughness(float roughness) { m_Roughness = roughness; }
    inline void SetMetalness(float metalness) { m_Metalness = metalness; }
    inline void SetAlbedoMap(const std::shared_ptr<Texture>& albedoMap) { m_AlbedoMap = albedoMap; }
    inline void SetNormalMap(const std::shared_ptr<Texture>& normalMap) { m_NormalMap = normalMap; }
    inline void SetRoughnessMap(const std::shared_ptr<Texture>& roughnessMap) { m_RoughnessMap = roughnessMap; }
    inline void SetMetalnessMap(const std::shared_ptr<Texture>& metalnessMap) { m_MetalnessMap = metalnessMap; }

    inline bool GetFlag(MaterialFlags flag) const { return (m_Flags & flag) != MaterialFlags::None; }
    inline const glm::vec3& GetAlbedoColor() const { return m_AlbedoColor; }
    inline float GetRoughness() const { return m_Roughness; }
    inline float GetMetalness() const { return m_Metalness; }
    inline const std::shared_ptr<Texture>& GetAlbedoMap() const { return m_AlbedoMap; }
    inline const std::shared_ptr<Texture>& GetNormalMap() const { return m_NormalMap; }
    inline const std::shared_ptr<Texture>& GetRoughnessMap() const { return m_RoughnessMap; }
    inline const std::shared_ptr<Texture>& GetMetalnessMap() const { return m_MetalnessMap; }
private:
    MaterialFlags m_Flags = MaterialFlags::None;
    glm::vec3 m_AlbedoColor = glm::vec3(1.0f);
    float m_Roughness = 0.5f;
    float m_Metalness = 0.5f;
    std::shared_ptr<Texture> m_AlbedoMap;
    std::shared_ptr<Texture> m_NormalMap;
    std::shared_ptr<Texture> m_RoughnessMap;
    std::shared_ptr<Texture> m_MetalnessMap;
};