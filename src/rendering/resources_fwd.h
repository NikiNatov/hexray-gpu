#pragma once
#include <memory>

class Buffer;
typedef std::shared_ptr<Buffer> BufferPtr;

class Mesh;
typedef std::shared_ptr<Mesh> MeshPtr;

class Texture;
typedef std::shared_ptr<Texture> TexturePtr;

class Material;
typedef std::shared_ptr<Material> MaterialPtr;