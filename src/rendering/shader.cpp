#include "shader.h"
#include <fstream>

// ------------------------------------------------------------------------------------------------------------------------------------
Shader::Shader(const std::string& filepath)
{
    HEXRAY_ASSERT_MSG(std::filesystem::exists(filepath), "Could not find shader file: {}", filepath);

    std::ifstream ifs(filepath, std::ios::in | std::ios::binary | std::ios::ate);
    HEXRAY_ASSERT_MSG(ifs, "Could not open shader file: {}", filepath);

    uint32_t fileSize = ifs.tellg();
    m_ByteCode.BytecodeLength = fileSize;
    m_ByteCode.pShaderBytecode = new uint8_t[fileSize];

    ifs.seekg(0, std::ios::beg);
    ifs.read((char*)m_ByteCode.pShaderBytecode, fileSize);
    ifs.close();
}

// ------------------------------------------------------------------------------------------------------------------------------------
Shader::~Shader()
{
    delete[] m_ByteCode.pShaderBytecode;
}
