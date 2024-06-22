#pragma once

struct ShaderCompilerInput;
struct ShaderCompilerResult;


void CompileHLSL_DXC(const ShaderCompilerInput& input, ShaderCompilerResult& result);
