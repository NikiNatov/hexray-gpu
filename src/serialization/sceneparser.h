#pragma once
#include "rendering/resources_fwd.h"

class Material;
class Texture;
class Mesh;


class SceneParser
{
public:
	virtual ~SceneParser() {}
	// All these methods are intended to be called by SceneElement implementations:
	virtual MaterialPtr FindMaterialByName(const char* name) const = 0;
	virtual TexturePtr FindTextureByName(const char* name) const = 0;
	virtual MeshPtr FindMeshByName(const char* name) const = 0;


	/**
	 * ResolveFullPath() tries to find a file (or folder), by appending the given path to the directory, where
	 * the scene file resides. The idea is that all external files (textures, meshes, etc.) are
	 * stored in the same dir where the scene file (*.qdmg) resides, and the paths to that external
	 * files do not mention any directories, just the file names.
	 *
	 * @param path (input-output) - Supply the given file name (as given in the scene file) here.
	 *                              If the function succeeds, this will return the full path to a file
	 *                              with that name, if it's found.
	 * @returns true on success, false on failure (file not found).
	 */
	virtual bool ResolveFullPath(char* path) = 0;
};
