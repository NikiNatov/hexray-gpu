#pragma once

#include "core/core.h"
#include "scene/scene.h"

class SceneSerializer
{
public:
    static void Serialize(const std::filesystem::path& filepath, const std::shared_ptr<Scene>& scene);
    static bool Deserialize(const std::filesystem::path& filepath, std::shared_ptr<Scene>& scene);
};