#include "HLSLReflection.h"

#include "core/core.h"
#include "shaderCompiler/shaderCompiler.h"

#include <dxc/d3d12shader.h>

namespace
{
template <typename ShaderReflectionVariable, typename SHADER_VARIABLE_DESC, typename ShaderReflectionConstantBuffer, typename SHADER_BUFFER_DESC>
std::vector<ConstantBufferVariable> ExtractCBVariables(ShaderReflectionConstantBuffer* d3dBuffer, const SHADER_BUFFER_DESC& bufferDesc)
{
	std::vector<ConstantBufferVariable> result(bufferDesc.Variables);
	for (uint32_t j = 0; j < bufferDesc.Variables; j++)
	{
		ShaderReflectionVariable* d3dVariable = d3dBuffer->GetVariableByIndex(j);
		SHADER_VARIABLE_DESC variableDesc;
		d3dVariable->GetDesc(&variableDesc);

		ConstantBufferVariable& variable = result[j];
		variable.Name = variableDesc.Name;
		variable.Offset = static_cast<uint16_t>(variableDesc.StartOffset);
		variable.Size = static_cast<uint16_t>(variableDesc.Size);
	}
	return result;
}
}


template <typename SHADER_DESC,
          typename ShaderReflectionConstantBuffer,
          typename SHADER_BUFFER_DESC,
          typename SHADER_INPUT_BIND_DESC,
          typename ShaderReflectionVariable,
          typename SHADER_VARIABLE_DESC,
          typename ShaderReflection>
void ExtractHLSLReflection_Internal(ShaderCompilerResult& result, const ComPtr<ShaderReflection>& reflector)
{
	SHADER_DESC shaderDesc;
	reflector->GetDesc(&shaderDesc);

	result.InstructionCount = shaderDesc.InstructionCount;

	result.ConstantBuffers.resize(shaderDesc.ConstantBuffers);
	for (uint32_t i = 0; i < shaderDesc.ConstantBuffers; i++)
	{
		ShaderReflectionConstantBuffer* d3dBuffer = reflector->GetConstantBufferByIndex(i);
		SHADER_BUFFER_DESC bufferDesc;
		d3dBuffer->GetDesc(&bufferDesc);

		ShaderConstantBuffer& buffer = result.ConstantBuffers[i];
		buffer.Name = bufferDesc.Name;
		buffer.Size = bufferDesc.Size;
		buffer.Variables = ExtractCBVariables<ShaderReflectionVariable, SHADER_VARIABLE_DESC>(d3dBuffer, bufferDesc);
	}

	result.Resources.resize(shaderDesc.BoundResources);
	for (uint32_t i = 0; i < shaderDesc.BoundResources; i++)
	{
		SHADER_INPUT_BIND_DESC resDesc;
		reflector->GetResourceBindingDesc(i, &resDesc);

		ShaderResource& resource = result.Resources[i];
		resource.Name = resDesc.Name;
		resource.Type = resDesc.Type;
		resource.BindPoint = uint8_t(resDesc.BindPoint);
		resource.BindCount = uint16_t(resDesc.BindCount);
	}
}

void ExtractHLSLReflection(ShaderCompilerResult& result, const ComPtr<ID3D12ShaderReflection>& reflector)
{
	ExtractHLSLReflection_Internal<D3D12_SHADER_DESC,
	                               ID3D12ShaderReflectionConstantBuffer,
	                               D3D12_SHADER_BUFFER_DESC,
	                               D3D12_SHADER_INPUT_BIND_DESC,
	                               ID3D12ShaderReflectionVariable,
	                               D3D12_SHADER_VARIABLE_DESC>(result, reflector);
}
