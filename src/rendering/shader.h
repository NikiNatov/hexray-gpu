#pragma once

#include "core/core.h"
#include "directx12.h"

class Shader
{
public:
    Shader(const std::string& filepath);
    ~Shader();

    inline const D3D12_SHADER_BYTECODE& GetByteCode() const { return m_ByteCode; }
private:
    D3D12_SHADER_BYTECODE m_ByteCode;
};