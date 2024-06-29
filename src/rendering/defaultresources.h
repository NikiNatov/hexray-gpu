#pragma once

#include "core/core.h"
#include "core/utils.h"
#include "rendering/texture.h"
#include "rendering/mesh.h"

#include <unordered_map>
#include <gtx/hash.hpp>


class DefaultResources
{
public:
    // Textures
    static TexturePtr BlackTexture;
    static TexturePtr BlackTextureCube;
    static TexturePtr ErrorTexture;
    static TexturePtr ErrorTextureCube;
    static TexturePtr WhiteTexture;
    static TexturePtr WhiteTextureCube;

    // Materials
    static MaterialPtr DefaultMaterial;

    // Meshes
    static MeshPtr QuadMesh;

    static TexturePtr GetColorTexture(const glm::vec4& color);
    static TexturePtr GetCheckerTexture(const glm::vec4& colorA, const glm::vec4& colorB);

    static void Initialize();
    static void Shutdown();

private:
    static std::unordered_map<glm::vec4, TexturePtr> m_RuntimeColorTextures;
    static std::unordered_map<std::pair<glm::vec4, glm::vec4>, TexturePtr> m_RuntimeCheckerTextures;
};