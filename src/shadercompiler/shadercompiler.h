#pragma once

#include <string>
#include <vector>
#include <filesystem>


enum ShaderCompileFlags
{
	SCF_Debug = 0x01,
	SCF_Release = 0x02,
	SCF_MatrixPackRowMajor = 0x04,
};

struct ConstantBufferVariable
{
	std::string Name;
	uint16_t Offset = 0;
	uint16_t Size = 0;
};

struct ShaderConstantBuffer
{
	std::string Name;
	uint32_t Size;
	std::vector<ConstantBufferVariable> Variables;
};

struct ShaderResource
{
	std::string Name;
	int32_t Type /*D3D_SHADER_INPUT_TYPE*/;
	uint8_t BindPoint;
	uint16_t BindCount;
};

struct ShaderCompilerInput
{
	std::filesystem::path SourcePath;
	std::filesystem::path DestinationDirectory;
	std::wstring EntryPoint;
	std::wstring ShaderVersion;
	ShaderCompileFlags Flags;
	std::vector<std::wstring> Defines;
};

struct ShaderCompilerResult
{
	std::vector<uint8_t> Code;
	std::vector<std::string> Errors;
	std::vector<ShaderConstantBuffer> ConstantBuffers;
	std::vector<ShaderResource> Resources;
	uint32_t InstructionCount;
};


class ShaderCompiler
{
public:
	static bool Compile(const ShaderCompilerInput& input, ShaderCompilerResult& result);
};
