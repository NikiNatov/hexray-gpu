#pragma once
#include "rendering/texturedescription.h"
#include "rendering/resources_fwd.h"

#include <string>

class Texture;

class TextureLoader
{
public:
	struct Result : public TextureDescription
	{
		uint8_t* Pixels = nullptr;

		~Result()
		{
			// todo: will lead to terrible bugs if we don't wait for upload to finish
			free(Pixels);
		}
	};

	static bool LoadFromFile(const std::string& file, TextureLoader::Result& outResult);
	static TexturePtr LoadFromFile(const std::string& file);
	static TexturePtr LoadFromCompressedData(uint8_t* data, uint32_t dataSize);
private:
	static bool LoadDDS(uint8_t* data, uint32_t size, TextureLoader::Result& outResult);
	static bool LoadSTBI(uint8_t* data, uint32_t size, TextureLoader::Result& outResult);
};
