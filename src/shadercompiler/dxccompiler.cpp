#include "dxccompiler.h"

#include "rendering/directx12.h"
#include "core/core.h"
#include "core/logger.h"
#include "core/utils.h"
#include "shadercompiler/shadercompiler.h"
#include "shadercompiler/hlslreflection.h"

#include <dxc/d3d12shader.h>
#include <dxc/dxcapi.h>


#ifndef HR
#define HR(Action)                                                                      \
    if (FAILED((Action)))                                                               \
    {                                                                                   \
        HEXRAY_ERROR("Failure with HRESULT of {}", static_cast<unsigned int>((Action)));\
        UNREACHED;                                                                      \
    }
#endif


void CompileHLSL_DXC(const ShaderCompilerInput& input, ShaderCompilerResult& result)
{
	const char* dllName = "dxcompiler.dll";
	static auto dxcLibrary = LoadLibraryA(dllName);

	if (!dllName)
	{
		result.Errors.emplace_back(Printf("Couldn't find %s to compile HLSL code", dllName));
		return;
	}
	
	DxcCreateInstanceProc DxcCreateInstanceFunc = (DxcCreateInstanceProc)GetProcAddress(dxcLibrary, "DxcCreateInstance");
	if (!DxcCreateInstanceFunc)
	{
		result.Errors.emplace_back(Printf("Couldn't find dll functions in %s to compile HLSL code", dllName));
		if (!DxcCreateInstanceFunc) result.Errors.emplace_back("Missing DxcCreateInstance()");
		return;
	}

	ComPtr<IDxcUtils> utils;
	ComPtr<IDxcCompiler3> compiler;
	
	DxcCreateInstanceFunc(CLSID_DxcUtils, IID_PPV_ARGS(utils.GetAddressOf()));
	DxcCreateInstanceFunc(CLSID_DxcCompiler, IID_PPV_ARGS(compiler.GetAddressOf()));
	
	if (!utils || !compiler)
	{
		result.Errors.emplace_back(Printf("Couldn't find dxc types in %s to compile HLSL code", dllName));
		if (!utils) result.Errors.emplace_back("Missing IDxcUtils");
		if (!compiler) result.Errors.emplace_back("Missing IDxcCompiler3");
		return;
	}
	
	std::vector<const wchar_t*> args;
	
	args.emplace_back(L"-E");
	args.emplace_back(input.EntryPoint.c_str());
	
	//-T for the target profile (eg. ps_6_0)
	args.emplace_back(L"-T");
	args.emplace_back(input.ShaderVersion.c_str());
	
	if (input.Flags & SCF_Debug)
	{
		args.emplace_back(DXC_ARG_DEBUG);
		args.emplace_back(DXC_ARG_OPTIMIZATION_LEVEL0);
	}
	
	if (input.Flags & SCF_Release)
	{
		args.emplace_back(DXC_ARG_OPTIMIZATION_LEVEL3);
		args.emplace_back(L"-Qstrip_debug");
	}
	
	args.emplace_back(input.Flags & SCF_MatrixPackRowMajor ? DXC_ARG_PACK_MATRIX_ROW_MAJOR : DXC_ARG_PACK_MATRIX_COLUMN_MAJOR);

	std::wstring fileName = input.SourcePath.stem().wstring();
	args.emplace_back(fileName.c_str());


	// -I Include directory
	std::wstring directory = input.SourcePath.parent_path().wstring();
	args.emplace_back(L"-I");
	args.emplace_back(directory.c_str());
	
	// L"-D", L"MYDEFINE=1"
	for (const std::wstring& define : input.Defines)
	{
		args.emplace_back(L"-D");
		args.emplace_back(define.c_str());
	}
	
	// Strip reflection into a separate blob.
	args.emplace_back(L"-Qstrip_reflect");
	
	std::string data;
	if (!ReadFile(input.SourcePath, data))
	{
		result.Errors.emplace_back(Printf("Couldn't find the file %s", input.SourcePath.c_str()));
		return;
	}
	
	DxcBuffer sourceBuffer;
	sourceBuffer.Ptr = data.data();
	sourceBuffer.Size = data.size();
	sourceBuffer.Encoding = DXC_CP_ACP;

	ComPtr<IDxcIncludeHandler> includeHandler;
	HR(utils->CreateDefaultIncludeHandler(includeHandler.GetAddressOf()));

	ComPtr<IDxcResult> compileResult;
	HR(compiler->Compile(&sourceBuffer, args.data(), args.size(), includeHandler.Get(), IID_PPV_ARGS(compileResult.GetAddressOf())));

	ComPtr<IDxcBlobUtf8> errors;
	HR(compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.GetAddressOf()), nullptr));

	// IDxcCompiler3::Compile will always return an error buffer, but its length will be zero if there are no warnings or errors.
	if (!errors || errors->GetBufferSize())
	{
		std::string error = std::string(errors->GetStringPointer(), errors->GetStringLength());
		if (error.find("error:") != std::string::npos)
		{
			result.Errors.emplace_back(error);
			return;
		}
		else
		{
			HEXRAY_WARNING(error);
		}
	}

	HRESULT hrStatus;
	if (FAILED(compileResult->GetStatus(&hrStatus)) || FAILED(hrStatus))
	{
		result.Errors.emplace_back("Shader compilation Failed\n");
		return;
	}

	ComPtr<IDxcBlob> resultBlob;
	HR(compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(resultBlob.GetAddressOf()), nullptr));
	if (!resultBlob)
	{
		result.Errors.emplace_back("Couldn't get compile output");
		return;
	}

	HRESULT hr;
	ComPtr<IDxcBlob> reflectionBlob;
	hr = compileResult->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(reflectionBlob.GetAddressOf()), nullptr);
	if (FAILED(hr) || !reflectionBlob)
	{
		result.Errors.emplace_back("Couldn't get reflection output");
		return;
	}

	DxcBuffer reflectionBuffer;
	reflectionBuffer.Ptr = reflectionBlob->GetBufferPointer();
	reflectionBuffer.Size = reflectionBlob->GetBufferSize();
	reflectionBuffer.Encoding = DXC_CP_ACP;

	// TODO: Enable reflection
	//ComPtr<ID3D12ShaderReflection> reflector;
	//hr = utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(reflector.GetAddressOf()));
	//if (FAILED(hr) || !reflector)
	//{
	//	result.Errors.emplace_back("Couldn't reflect the shader code");
	//	return;
	//}
	//	
	//ExtractHLSLReflection(result, reflector);

	result.Code.resize(resultBlob->GetBufferSize());
	memcpy(result.Code.data(), resultBlob->GetBufferPointer(), resultBlob->GetBufferSize());
}
