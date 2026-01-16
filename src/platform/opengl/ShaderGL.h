/**
 * @file ShaderGL.h
 * @brief OpenGL着色器实现
 */

#pragma once

#include "platform/interface/IShaderImpl.h"
#include "TypeConverterGL.h"
#include <string>

#ifdef LRENGINE_ENABLE_OPENGL

namespace lrengine {
namespace render {
namespace gl {

/**
 * @brief OpenGL着色器实现
 */
class ShaderGL : public IShaderImpl {
public:
    ShaderGL();
    ~ShaderGL() override;

    // IShaderImpl接口
    bool Compile(const ShaderDescriptor& desc) override;
    void Destroy() override;
    bool IsCompiled() const override;
    const char* GetCompileError() const override;
    ShaderStage GetStage() const override;
    ResourceHandle GetNativeHandle() const override;

    // OpenGL特有方法
    GLuint GetShaderID() const { return m_shaderID; }

private:
    GLuint m_shaderID;
    ShaderStage m_stage;
    bool m_compiled;
    std::string m_compileError;
};

/**
 * @brief OpenGL着色器程序实现
 */
class ShaderProgramGL : public IShaderProgramImpl {
public:
    ShaderProgramGL();
    ~ShaderProgramGL() override;

    // IShaderProgramImpl接口
    bool Link(IShaderImpl** shaders, uint32_t count) override;
    void Destroy() override;
    bool IsLinked() const override;
    const char* GetLinkError() const override;
    void Use() override;
    int32_t GetUniformLocation(const char* name) override;
    void SetUniform1i(int32_t location, int32_t value) override;
    void SetUniform1f(int32_t location, float value) override;
    void SetUniform2f(int32_t location, float x, float y) override;
    void SetUniform3f(int32_t location, float x, float y, float z) override;
    void SetUniform4f(int32_t location, float x, float y, float z, float w) override;
    void SetUniformMatrix3fv(int32_t location, const float* value, bool transpose = false) override;
    void SetUniformMatrix4fv(int32_t location, const float* value, bool transpose = false) override;
    ResourceHandle GetNativeHandle() const override;

    // OpenGL特有方法
    GLuint GetProgramID() const { return m_programID; }
    void BindUniformBlock(const char* blockName, uint32_t bindingPoint);

private:
    GLuint m_programID;
    bool m_linked;
    std::string m_linkError;
};

} // namespace gl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGL
