#include "textureloader.h"
#include "resourceloaders/ddsloader.h"
#include "rendering/graphicscontext.h"

#define STB_IMAGE_IMPLEMENTATION

#include <fstream>
#include <stb/stb_image.h>


bool TextureLoader::LoadFromFile(const std::string& file, Result& outResult)
{
	std::ifstream ifs(file, std::ios::in | std::ios::binary | std::ios::ate);
	HEXRAY_ASSERT_MSG(ifs, "Could not texture file: {}", file);

	uint32_t dataSize = ifs.tellg();
	uint8_t* data = new uint8_t[dataSize];

	ifs.seekg(0, std::ios::beg);
	ifs.read((char*)data, dataSize);
	ifs.close();

	if (file.find(".dds") != std::string::npos)
	{
		return LoadDDS(data, dataSize, outResult);
	}

	return LoadSTBI(data, dataSize, outResult);
}

std::shared_ptr<Texture> TextureLoader::LoadFromFile(const std::string& file)
{
	TextureLoader::Result result;
	if (!LoadFromFile(file, result))
	{
		return nullptr;
	}

	std::shared_ptr<Texture> texture = std::make_shared<Texture>(result);
	GraphicsContext::GetInstance()->UploadTextureData(texture.get(), result.Pixels);

	// todo: will lead to terrible bugs if we don't wait for upload to finish
	delete[] result.Pixels;
	return texture;
}

bool TextureLoader::LoadDDS(uint8_t* data, uint32_t size, Result& outResult)
{
	const DDSContents* dds = DDSContents::FromData(data, size);
	if (dds && dds->IsValid())
	{
		outResult.Format = dds->GetPixelFormat();
		outResult.Type = dds->GetTextureType();
		outResult.ArrayLevels = dds->GetArrayLevels();
		outResult.Width = dds->GetWidth();
		outResult.Height = dds->GetHeight();
		outResult.Depth = dds->GetDepth();
		outResult.MipLevels = dds->GetMipCount();
		outResult.IsCubeMap = dds->IsCubemap();

		NOT_IMPLEMENTED_BECAUSE("Need to calc bytes");
		uint32_t calculatedBytes = dds->GetTotalBytes();
		outResult.Pixels = new uint8_t[calculatedBytes];
		memcpy(outResult.Pixels, dds->GetPixels(), calculatedBytes);
		return true;
	}

	return false;
}

bool TextureLoader::LoadSTBI(uint8_t* data, uint32_t size, Result& outResult)
{
	outResult.Type = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	int32_t channels;
	int32_t width, height;
	if (stbi_info_from_memory(data, int32_t(size), &width, &height, &channels))
	{
		if (stbi_is_hdr_from_memory(data, int32_t(size)))
		{
			switch (channels)
			{
			case 1: outResult.Format = DXGI_FORMAT_R32_FLOAT; break;
			case 2: outResult.Format = DXGI_FORMAT_R32G32_FLOAT; break;
			case 3: outResult.Format = DXGI_FORMAT_R32G32B32_FLOAT; break;
			case 4: outResult.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
			}
			outResult.Pixels = (uint8_t*)stbi_loadf_from_memory(data, int32_t(size), &width, &height, &channels, channels);
		}
		else
		{
			if (channels == 3)
			{
				// Converts RGB to RGBA, since there are no RGB8 formats.
				channels = 4;
			}

			switch (channels)
			{
			case 1: outResult.Format = DXGI_FORMAT_R8_UNORM; break;
			case 2: outResult.Format = DXGI_FORMAT_R8G8_UNORM; break;
			case 4: outResult.Format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
			}

			outResult.Pixels = stbi_load_from_memory(data, int32_t(size), &width, &height, &channels, channels);
		}
	}

	if (!outResult.Pixels && stbi_failure_reason())
	{
		HEXRAY_ERROR("{}", stbi_failure_reason());
		return false;
	}

	outResult.Width = uint32_t(width);
	outResult.Height = uint32_t(height);
	return true;
}
