#include "ShaderCompiler.h"

#include "dxccompiler.h"
#include "core/logger.h"



bool ShaderCompiler::Compile(const ShaderCompilerInput& input, ShaderCompilerResult& result)
{
	HEXRAY_INFO("Compiling shader {}", input.SourcePath.string());

	CompileHLSL_DXC(input, result);

	return result.Errors.empty();
}
