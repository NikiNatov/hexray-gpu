#pragma once

#include "core/core.h"
#include "rendering/texture.h"
#include "rendering/mesh.h"

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
    static MaterialPtr ErrorMaterial;

    // Meshes
    static MeshPtr TriangleMesh;
    static MeshPtr QuadMesh;

    static void Initialize();
    static void Shutdown();
};