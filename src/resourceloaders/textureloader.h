#pragma once
#include "rendering/texture.h"

#include <string>

class TextureLoader
{
public:
	struct Result : public TextureDescription
	{
		std::string Name;
		uint8_t* Pixels = nullptr;
	};

	static bool LoadFromFile(const std::string& file, TextureLoader::Result& outResult);
	static std::shared_ptr<Texture> LoadFromFile(const std::string& file);
	static std::shared_ptr<Texture> LoadFromCompressedData(uint8_t* data, uint32_t dataSize);
private:
	static bool LoadDDS(uint8_t* data, uint32_t size, TextureLoader::Result& outResult);
	static bool LoadSTBI(uint8_t* data, uint32_t size, TextureLoader::Result& outResult);
};
