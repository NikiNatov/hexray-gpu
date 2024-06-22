#pragma once
#include "rendering/directx12.h"

struct ShaderCompilerResult;

struct ID3D11ShaderReflection;
struct ID3D12ShaderReflection;

// D3D12
void ExtractHLSLReflection(ShaderCompilerResult& result, const ComPtr<ID3D12ShaderReflection>& reflector);

