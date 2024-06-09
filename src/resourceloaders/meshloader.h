#pragma once

#include "core/core.h"

#include "rendering/mesh.h"
#include "rendering/resources_fwd.h"

class MeshLoader
{
public:
    static MeshPtr LoadFromFile(const std::filesystem::path& filePath);
private:
    static MeshPtr LoadAssimp(const std::filesystem::path& filePath);
};