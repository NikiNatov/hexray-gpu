#pragma once
#include "core/core.h"
#include "rendering/directx12.h"

#include <d3d10.h>

#define DDS_RGB       0x00000040
#define DDS_LUMINANCE 0x00020000
#define DDS_ALPHA     0x00000002
#define DDS_BUMPDUDV  0x00080000
#define DDS_FOURCC    0x00000004
#define ISBITMASK(r, g, b, a) (DDSFormat.RMask == r && DDSFormat.GMask == g && DDSFormat.BMask == b && DDSFormat.AMask == a)
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
                ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))

// https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds-pguide
// https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-header-dxt10
struct DDSHeaderDXT10
{
	DXGI_FORMAT DxgiFormat;
	D3D10_RESOURCE_DIMENSION ResourceDimension;
	uint32_t MiscFlag;
	uint32_t ArraySize;
	uint32_t MiscFlags2;
};

// https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-pixelformat
struct DDSPixelFormat
{
	uint32_t Size;
	uint32_t Flags;
	uint32_t FourCC;
	uint32_t RGBBits;
	uint32_t RMask;
	uint32_t GMask;
	uint32_t BMask;
	uint32_t AMask;
};

// https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-header
struct DDSHeader
{
	uint32_t Size;
	uint32_t Flags;
	uint32_t Height;
	uint32_t Width;
	uint32_t PitchOrLinearSize;
	uint32_t Depth;
	uint32_t MipCount;
	uint32_t Reserved1[11];
	DDSPixelFormat DDSFormat;
	uint32_t Caps;
	uint32_t Caps2;
	uint32_t Caps3;
	uint32_t Caps4;
	uint32_t Reserved2;
};


inline size_t BitsPerPixel(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_R32G32B32A32_TYPELESS:
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
		return 128;

	case DXGI_FORMAT_R32G32B32_TYPELESS:
	case DXGI_FORMAT_R32G32B32_FLOAT:
	case DXGI_FORMAT_R32G32B32_UINT:
	case DXGI_FORMAT_R32G32B32_SINT:
		return 96;

	case DXGI_FORMAT_R16G16B16A16_TYPELESS:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R32G32_TYPELESS:
	case DXGI_FORMAT_R32G32_FLOAT:
	case DXGI_FORMAT_R32G32_UINT:
	case DXGI_FORMAT_R32G32_SINT:
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		return 64;

	case DXGI_FORMAT_R10G10B10A2_TYPELESS:
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UINT:
	case DXGI_FORMAT_R11G11B10_FLOAT:
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_R16G16_TYPELESS:
	case DXGI_FORMAT_R16G16_FLOAT:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R16G16_UINT:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R16G16_SINT:
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
	case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
	case DXGI_FORMAT_R8G8_B8G8_UNORM:
	case DXGI_FORMAT_G8R8_G8B8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		return 32;

	case DXGI_FORMAT_R8G8_TYPELESS:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R8G8_SINT:
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
	case DXGI_FORMAT_B5G6R5_UNORM:
	case DXGI_FORMAT_B5G5R5A1_UNORM:
	case DXGI_FORMAT_B4G4R4A4_UNORM:
		return 16;

	case DXGI_FORMAT_R8_TYPELESS:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
	case DXGI_FORMAT_A8_UNORM:
		return 8;

	case DXGI_FORMAT_R1_UNORM:
		return 1;

	case DXGI_FORMAT_BC1_TYPELESS:
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC4_TYPELESS:
	case DXGI_FORMAT_BC4_UNORM:
	case DXGI_FORMAT_BC4_SNORM:
		return 4;

	case DXGI_FORMAT_BC2_TYPELESS:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_TYPELESS:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_BC5_TYPELESS:
	case DXGI_FORMAT_BC5_UNORM:
	case DXGI_FORMAT_BC5_SNORM:
	case DXGI_FORMAT_BC6H_TYPELESS:
	case DXGI_FORMAT_BC6H_UF16:
	case DXGI_FORMAT_BC6H_SF16:
	case DXGI_FORMAT_BC7_TYPELESS:
	case DXGI_FORMAT_BC7_UNORM:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		return 8;

	default:
		return 0;
	}
}

struct DDSContents
{
	enum
	{
		MagicNumber = 0x20534444
	};

	uint32_t Magic;
	DDSHeader Header;
	DDSHeaderDXT10 HeaderDXT10;

	static const DDSContents* FromData(uint8_t* data, uint32_t size)
	{
		return sizeof(DDSContents) < size ? reinterpret_cast<DDSContents*>(data) : nullptr;
	}

	bool IsValid() const
	{
		return Magic == MagicNumber && Header.Size == sizeof(DDSHeader) && Header.DDSFormat.Size == sizeof(DDSPixelFormat);
	}

	bool HasDXT10() const
	{
		return (Header.DDSFormat.Flags & DDS_FOURCC) && Header.DDSFormat.FourCC == MAKEFOURCC('D', 'X', '1', '0');
	}

	uint8_t* GetPixels() const
	{
		uint8_t* ptrOfThisStruct = (uint8_t*)this;
		return ptrOfThisStruct + (HasDXT10() ? offsetof(DDSContents, HeaderDXT10) + sizeof(DDSHeaderDXT10) : offsetof(DDSContents, HeaderDXT10));
	}

	uint32_t GetArrayLevels() const
	{
		if (HasDXT10())
		{
			return HeaderDXT10.ArraySize;
		}
		return IsCubemap() ? 6 : 1;
	}

	uint32_t GetWidth() const
	{
		return Header.Flags & 0x4 ? Header.Width : 1;
	}

	uint32_t GetHeight() const
	{
		return Header.Flags & 0x2 ? Header.Height : 1;
	}

	uint32_t GetDepth() const
	{
		return Header.Flags & 0x800000 ? Header.Depth : 1;
	}

	uint32_t GetMipCount() const
	{
		return Header.Flags & 0x20000 ? Header.MipCount : 1;
	}

	uint32_t GetTotalBytes() const
	{
		return 0;
	}

	bool IsCubemap() const
	{
		return HeaderDXT10.MiscFlag & D3D10_RESOURCE_MISC_TEXTURECUBE;
	}

	D3D12_RESOURCE_DIMENSION GetTextureType() const
	{
		if (HasDXT10())
		{
			if (IsCubemap())
			{
				return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			}

			switch (HeaderDXT10.ResourceDimension)
			{
			case D3D10_RESOURCE_DIMENSION_TEXTURE1D: return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
			case D3D10_RESOURCE_DIMENSION_TEXTURE2D: return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			case D3D10_RESOURCE_DIMENSION_TEXTURE3D: return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
			}
		}
		else
		{
			if (GetHeight() == 1)
			{
				return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
			}

			if (Header.Flags & 0x00800000)
			{
				return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
			}

			if (Header.Caps2 & 0x00000200)
			{
				return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			}

			return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		}

		UNREACHED;
		return D3D12_RESOURCE_DIMENSION(-1);
	}

	DXGI_FORMAT GetPixelFormat() const
	{
		const DDSPixelFormat& DDSFormat = Header.DDSFormat;
		if (DDSFormat.Flags & DDS_RGB)
		{
			switch (DDSFormat.RGBBits)
			{
			case 32:
				if (ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
				{
					return DXGI_FORMAT_R8G8B8A8_UNORM;
				}

				if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000))
				{
					return DXGI_FORMAT_B8G8R8A8_UNORM;
				}

				if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000))
				{
					return DXGI_FORMAT_B8G8R8X8_UNORM;
				}

				if (ISBITMASK(0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000))
				{
					return DXGI_FORMAT_R10G10B10A2_UNORM;
				}

				if (ISBITMASK(0x0000ffff, 0xffff0000, 0x00000000, 0x00000000))
				{
					return DXGI_FORMAT_R16G16_UNORM;
				}

				if (ISBITMASK(0xffffffff, 0x00000000, 0x00000000, 0x00000000))
				{
					return DXGI_FORMAT_R32_FLOAT;
				}
				break;

			case 24:
				// No 24bpp DXGI formats aka D3DFMT_R8G8B8
				break;

			case 16:
				if (ISBITMASK(0x7c00, 0x03e0, 0x001f, 0x8000))
				{
					return DXGI_FORMAT_B5G5R5A1_UNORM;
				}
				if (ISBITMASK(0xf800, 0x07e0, 0x001f, 0x0000))
				{
					return DXGI_FORMAT_B5G6R5_UNORM;
				}

				if (ISBITMASK(0x0f00, 0x00f0, 0x000f, 0xf000))
				{
					return DXGI_FORMAT_B4G4R4A4_UNORM;
				}
				break;
			}
		}
		else if (DDSFormat.Flags & DDS_LUMINANCE)
		{
			if (DDSFormat.RGBBits == 8)
			{
				if (ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x00000000))
				{
					return DXGI_FORMAT_R8_UNORM;
				}
			}

			if (DDSFormat.RGBBits == 16)
			{
				if (ISBITMASK(0x0000ffff, 0x00000000, 0x00000000, 0x00000000))
				{
					return DXGI_FORMAT_R16_UNORM; // D3DX10/11 writes this out as DX10 extension.
				}
				if (ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x0000ff00))
				{
					return DXGI_FORMAT_R8G8_UNORM; // D3DX10/11 writes this out as DX10 extension.
				}
			}
		}
		else if (DDSFormat.Flags & DDS_ALPHA)
		{
			if (DDSFormat.RGBBits == 8)
			{
				return DXGI_FORMAT_A8_UNORM;
			}
		}
		else if (DDSFormat.Flags & DDS_FOURCC)
		{
			if (MAKEFOURCC('D', 'X', 'T', '1') == DDSFormat.FourCC)
			{
				return DXGI_FORMAT_BC1_UNORM;
			}
			if (MAKEFOURCC('D', 'X', 'T', '3') == DDSFormat.FourCC)
			{
				return DXGI_FORMAT_BC2_UNORM;
			}
			if (MAKEFOURCC('D', 'X', 'T', '5') == DDSFormat.FourCC)
			{
				return DXGI_FORMAT_BC3_UNORM;
			}
			if (MAKEFOURCC('D', 'X', 'T', '2') == DDSFormat.FourCC)
			{
				return DXGI_FORMAT_BC2_UNORM;
			}
			if (MAKEFOURCC('D', 'X', 'T', '4') == DDSFormat.FourCC)
			{
				return DXGI_FORMAT_BC3_UNORM;
			}

			if (MAKEFOURCC('A', 'T', 'I', '1') == DDSFormat.FourCC)
			{
				return DXGI_FORMAT_BC4_UNORM;
			}
			if (MAKEFOURCC('B', 'C', '4', 'U') == DDSFormat.FourCC)
			{
				return DXGI_FORMAT_BC4_UNORM;
			}
			if (MAKEFOURCC('B', 'C', '4', 'S') == DDSFormat.FourCC)
			{
				return DXGI_FORMAT_BC4_SNORM;
			}
			if (MAKEFOURCC('A', 'T', 'I', '2') == DDSFormat.FourCC)
			{
				return DXGI_FORMAT_BC5_UNORM;
			}
			if (MAKEFOURCC('B', 'C', '5', 'U') == DDSFormat.FourCC)
			{
				return DXGI_FORMAT_BC5_UNORM;
			}
			if (MAKEFOURCC('B', 'C', '5', 'S') == DDSFormat.FourCC)
			{
				return DXGI_FORMAT_BC5_SNORM;
			}
			if (MAKEFOURCC('R', 'G', 'B', 'G') == DDSFormat.FourCC)
			{
				return DXGI_FORMAT_R8G8_B8G8_UNORM;
			}
			if (MAKEFOURCC('G', 'R', 'G', 'B') == DDSFormat.FourCC)
			{
				return DXGI_FORMAT_G8R8_G8B8_UNORM;
			}

			switch (DDSFormat.FourCC)
			{
			case 36: return DXGI_FORMAT_R16G16B16A16_UNORM;
			case 110: return DXGI_FORMAT_R16G16B16A16_SNORM;
			case 111: return DXGI_FORMAT_R16_FLOAT;
			case 112: return DXGI_FORMAT_R16G16_FLOAT;
			case 113: return DXGI_FORMAT_R16G16B16A16_FLOAT;
			case 114: return DXGI_FORMAT_R32_FLOAT;
			case 115: return DXGI_FORMAT_R32G32_FLOAT;
			case 116: return DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
		}

		return DXGI_FORMAT_UNKNOWN;
	}
};


#undef DDS_RGB
#undef DDS_LUMINANCE
#undef DDS_ALPHA
#undef DDS_BUMPDUDV
#undef DDS_FOURCC
#undef ISBITMASK
#undef MAKEFOURCC
