#pragma once

#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>
#include <dxgidebug.h>
#include "d3dx12.h"

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

#if defined(HEXRAY_DEBUG)

#define DXCall(hr)																																	   \
		if(FAILED(hr))																																   \
		{																																			   \
			LPSTR error;																													           \
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, \
				nullptr,																															   \
				hr,																																	   \
				0,																							                                           \
				(LPSTR)&error,																														   \
				512,																																   \
				nullptr																																   \
			);																																		   \
			HEXRAY_ERROR("DirectX Error:");																									           \
			HEXRAY_ERROR("[File]: {0}", __FILE__);																									   \
			HEXRAY_ERROR("[Line]: {0}", __LINE__);																									   \
			HEXRAY_ERROR("[Description]: {0}", error);																								   \
			__debugbreak();																															   \
		}

#else
#define DXCall(hr) hr
#endif