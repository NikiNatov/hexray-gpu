#pragma once

#include "core/core.h"

#include "rendering/mesh.h"

class MeshLoader
{
public:
    static std::shared_ptr<Mesh> LoadFromFile(const std::filesystem::path& filePath);
private:
    static std::shared_ptr<Mesh> LoadFBX(const std::filesystem::path& filePath);
};