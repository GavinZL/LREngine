/**
 * @file ShaderGL.cpp
 * @brief OpenGL着色器实现
 */

#include "ShaderGL.h"
#include "lrengine/core/LRError.h"
#include "lrengine/utils/LRLog.h"
#include <vector>

#ifdef LRENGINE_ENABLE_OPENGL

namespace lrengine {
namespace render {
namespace gl {

// ============================================================================
// ShaderGL
// ============================================================================

ShaderGL::ShaderGL() : m_shaderID(0), m_stage(ShaderStage::Vertex), m_compiled(false) {}

ShaderGL::~ShaderGL() { Destroy(); }

bool ShaderGL::Compile(const ShaderDescriptor& desc) {
    if (m_shaderID != 0) {
        Destroy();
    }

    m_stage           = desc.stage;
    GLenum shaderType = ToGLShaderStage(desc.stage);

    m_shaderID = glCreateShader(shaderType);
    if (m_shaderID == 0) {
        m_compileError = "Failed to create shader object";
        LR_SET_ERROR(ErrorCode::ShaderCompileFailed, m_compileError.c_str());
        return false;
    }

    const char* sourcePtr = desc.source;
    glShaderSource(m_shaderID, 1, &sourcePtr, nullptr);
    glCompileShader(m_shaderID);

    LR_LOG_DEBUG_F("OpenGL CompileShader: stage=%d, name=%s", (int)desc.stage,
                   desc.debugName ? desc.debugName : "unknown");

    GLint success;
    glGetShaderiv(m_shaderID, GL_COMPILE_STATUS, &success);

    if (!success) {
        GLint logLength;
        glGetShaderiv(m_shaderID, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength > 0) {
            std::vector<char> log(logLength);
            glGetShaderInfoLog(m_shaderID, logLength, nullptr, log.data());
            m_compileError = log.data();
        } else {
            m_compileError = "Unknown shader compilation error";
        }

        LR_SET_ERROR(ErrorCode::ShaderCompileFailed, m_compileError.c_str());
        LR_LOG_ERROR_F("OpenGL Shader Compile Failed: %s", m_compileError.c_str());
        glDeleteShader(m_shaderID);
        m_shaderID = 0;
        return false;
    }

    m_compiled = true;
    m_compileError.clear();
    return true;
}

void ShaderGL::Destroy() {
    if (m_shaderID != 0) {
        glDeleteShader(m_shaderID);
        m_shaderID = 0;
    }
    m_compiled = false;
}

bool ShaderGL::IsCompiled() const { return m_compiled; }

const char* ShaderGL::GetCompileError() const { return m_compileError.c_str(); }

ShaderStage ShaderGL::GetStage() const { return m_stage; }

ResourceHandle ShaderGL::GetNativeHandle() const {
    ResourceHandle handle;
    handle.glHandle = m_shaderID;
    return handle;
}

// ============================================================================
// ShaderProgramGL
// ============================================================================

ShaderProgramGL::ShaderProgramGL() : m_programID(0), m_linked(false) {}

ShaderProgramGL::~ShaderProgramGL() { Destroy(); }

bool ShaderProgramGL::Link(IShaderImpl** shaders, uint32_t count) {
    if (m_programID != 0) {
        Destroy();
    }

    if (shaders == nullptr || count == 0) {
        m_linkError = "No shaders provided";
        LR_SET_ERROR(ErrorCode::ShaderLinkFailed, m_linkError.c_str());
        return false;
    }

    m_programID = glCreateProgram();
    if (m_programID == 0) {
        m_linkError = "Failed to create program object";
        LR_SET_ERROR(ErrorCode::ShaderLinkFailed, m_linkError.c_str());
        return false;
    }

    // 附加所有着色器
    for (uint32_t i = 0; i < count; ++i) {
        if (shaders[i] == nullptr) {
            continue;
        }

        ShaderGL* glShader = static_cast<ShaderGL*>(shaders[i]);
        if (!glShader->IsCompiled()) {
            m_linkError = "Attempting to link with uncompiled shader";
            LR_SET_ERROR(ErrorCode::ShaderLinkFailed, m_linkError.c_str());
            glDeleteProgram(m_programID);
            m_programID = 0;
            return false;
        }

        glAttachShader(m_programID, glShader->GetShaderID());
    }

    // 链接程序
    glLinkProgram(m_programID);

    LR_LOG_DEBUG("OpenGL LinkProgram");

    GLint success;
    glGetProgramiv(m_programID, GL_LINK_STATUS, &success);

    if (!success) {
        GLint logLength;
        glGetProgramiv(m_programID, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength > 0) {
            std::vector<char> log(logLength);
            glGetProgramInfoLog(m_programID, logLength, nullptr, log.data());
            m_linkError = log.data();
        } else {
            m_linkError = "Unknown program link error";
        }

        LR_SET_ERROR(ErrorCode::ShaderLinkFailed, m_linkError.c_str());
        LR_LOG_ERROR_F("OpenGL Shader Link Failed: %s", m_linkError.c_str());
        glDeleteProgram(m_programID);
        m_programID = 0;
        return false;
    }

    // 分离着色器（链接后不再需要）
    for (uint32_t i = 0; i < count; ++i) {
        if (shaders[i] == nullptr) {
            continue;
        }
        ShaderGL* glShader = static_cast<ShaderGL*>(shaders[i]);
        glDetachShader(m_programID, glShader->GetShaderID());
    }

    m_linked = true;
    m_linkError.clear();
    return true;
}

void ShaderProgramGL::Destroy() {
    if (m_programID != 0) {
        glDeleteProgram(m_programID);
        m_programID = 0;
    }
    m_linked = false;
}

bool ShaderProgramGL::IsLinked() const { return m_linked; }

const char* ShaderProgramGL::GetLinkError() const { return m_linkError.c_str(); }

void ShaderProgramGL::Use() {
    if (m_programID != 0) {
        glUseProgram(m_programID);
        LR_LOG_DEBUG_F("OpenGL UseProgram: %d", m_programID);
    }
}

int32_t ShaderProgramGL::GetUniformLocation(const char* name) {
    if (m_programID == 0 || name == nullptr) {
        return -1;
    }
    return glGetUniformLocation(m_programID, name);
}

void ShaderProgramGL::SetUniform1i(int32_t location, int32_t value) {
    if (location >= 0) {
        glUniform1i(location, value);
    }
}

void ShaderProgramGL::SetUniform1f(int32_t location, float value) {
    if (location >= 0) {
        glUniform1f(location, value);
    }
}

void ShaderProgramGL::SetUniform2f(int32_t location, float x, float y) {
    if (location >= 0) {
        glUniform2f(location, x, y);
    }
}

void ShaderProgramGL::SetUniform3f(int32_t location, float x, float y, float z) {
    if (location >= 0) {
        glUniform3f(location, x, y, z);
    }
}

void ShaderProgramGL::SetUniform4f(int32_t location, float x, float y, float z, float w) {
    if (location >= 0) {
        glUniform4f(location, x, y, z, w);
    }
}

void ShaderProgramGL::SetUniformMatrix3fv(int32_t location, const float* value, bool transpose) {
    if (location >= 0 && value != nullptr) {
        glUniformMatrix3fv(location, 1, transpose ? GL_TRUE : GL_FALSE, value);
    }
}

void ShaderProgramGL::SetUniformMatrix4fv(int32_t location, const float* value, bool transpose) {
    if (location >= 0 && value != nullptr) {
        glUniformMatrix4fv(location, 1, transpose ? GL_TRUE : GL_FALSE, value);
    }
}

ResourceHandle ShaderProgramGL::GetNativeHandle() const {
    ResourceHandle handle;
    handle.glHandle = m_programID;
    return handle;
}

void ShaderProgramGL::BindUniformBlock(const char* blockName, uint32_t bindingPoint) {
    if (m_programID == 0 || blockName == nullptr) {
        return;
    }

    GLuint blockIndex = glGetUniformBlockIndex(m_programID, blockName);
    if (blockIndex != GL_INVALID_INDEX) {
        glUniformBlockBinding(m_programID, blockIndex, bindingPoint);
    }
}

} // namespace gl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGL
