#pragma once

#include "core/core.h"
#include "rendering/texture.h"
#include "rendering/mesh.h"

class DefaultResources
{
public:
    // Textures
    static std::shared_ptr<Texture> BlackTexture;
    static std::shared_ptr<Texture> BlackTextureCube;
    static std::shared_ptr<Texture> ErrorTexture;
    static std::shared_ptr<Texture> ErrorTextureCube;
    static std::shared_ptr<Texture> WhiteTexture;
    static std::shared_ptr<Texture> WhiteTextureCube;

    // Materials
    static std::shared_ptr<Material> DefaultMaterial;
    static std::shared_ptr<Material> ErrorMaterial;

    // Meshes
    static std::shared_ptr<Mesh> TriangleMesh;
    static std::shared_ptr<Mesh> QuadMesh;

    static void Initialize();
    static void Shutdown();
};