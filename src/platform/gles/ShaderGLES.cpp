/**
 * @file ShaderGLES.cpp
 * @brief OpenGL ES着色器实现
 */

#include "ShaderGLES.h"
#include "lrengine/core/LRError.h"
#include "lrengine/utils/LRLog.h"
#include <vector>
#include <cstring>

#ifdef LRENGINE_ENABLE_OPENGLES

namespace lrengine {
namespace render {
namespace gles {

// ============================================================================
// ShaderGLES
// ============================================================================

ShaderGLES::ShaderGLES()
    : m_shaderID(0)
    , m_stage(ShaderStage::Vertex)
    , m_compiled(false)
{
}

ShaderGLES::~ShaderGLES()
{
    Destroy();
}

std::string ShaderGLES::PreprocessGLSLES(const char* source, ShaderStage stage)
{
    if (!source) {
        return "";
    }
    
    std::string result;
    const char* sourceStart = source;
    
    // 跳过开头的空白字符
    while (*sourceStart && (*sourceStart == ' ' || *sourceStart == '\t' || 
           *sourceStart == '\n' || *sourceStart == '\r')) {
        sourceStart++;
    }
    
    // 检查是否已有版本声明
    bool hasVersion = (strncmp(sourceStart, "#version", 8) == 0);
    
    if (!hasVersion) {
        // 添加GLSL ES 3.00版本声明
        result = "#version 300 es\n";
    }
    
    // 添加精度声明
    // 注意：精度声明必须在版本声明之后，其他声明之前
    switch (stage) {
        case ShaderStage::Fragment:
            // Fragment着色器必须声明默认精度
            result += "precision highp float;\n";
            result += "precision highp int;\n";
            result += "precision highp sampler2D;\n";
            result += "precision highp sampler3D;\n";
            result += "precision highp samplerCube;\n";
            result += "precision highp sampler2DArray;\n";
            result += "precision highp sampler2DShadow;\n";
            break;
            
        case ShaderStage::Vertex:
            // Vertex着色器默认精度是highp，但显式声明更清晰
            result += "precision highp float;\n";
            result += "precision highp int;\n";
            break;
            
        case ShaderStage::Compute:
            // Compute着色器（ES 3.1+）
            result += "precision highp float;\n";
            result += "precision highp int;\n";
            result += "precision highp image2D;\n";
            break;
            
        default:
            break;
    }
    
    // 添加源码
    result += "\n";
    result += source;
    
    return result;
}

bool ShaderGLES::Compile(const ShaderDescriptor& desc)
{
    if (m_shaderID != 0) {
        Destroy();
    }

    m_stage = desc.stage;
    
    // 检查着色器阶段是否支持
    if (desc.stage == ShaderStage::Geometry || 
        desc.stage == ShaderStage::TessControl || 
        desc.stage == ShaderStage::TessEval) {
        m_compileError = "OpenGL ES 3.0 does not support Geometry/Tessellation shaders";
        LR_SET_ERROR(ErrorCode::ShaderCompileFailed, m_compileError.c_str());
        LR_LOG_ERROR_F("OpenGL ES Shader: %s", m_compileError.c_str());
        return false;
    }
    
    GLenum shaderType = ToGLESShaderStage(desc.stage);

    m_shaderID = glCreateShader(shaderType);
    if (m_shaderID == 0) {
        m_compileError = "Failed to create shader object";
        LR_SET_ERROR(ErrorCode::ShaderCompileFailed, m_compileError.c_str());
        return false;
    }

    // 预处理着色器源码（添加版本和精度声明）
    std::string processedSource = PreprocessGLSLES(desc.source, desc.stage);
    const char* sourcePtr = processedSource.c_str();
    
    glShaderSource(m_shaderID, 1, &sourcePtr, nullptr);
    glCompileShader(m_shaderID);

    LR_LOG_DEBUG_F("OpenGL ES CompileShader: stage=%d, name=%s", 
                   (int)desc.stage, desc.debugName ? desc.debugName : "unknown");

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
        LR_LOG_ERROR_F("OpenGL ES Shader Compile Failed: %s", m_compileError.c_str());
        
        // 打印预处理后的源码以便调试
        LR_LOG_DEBUG_F("Preprocessed source:\n%s", processedSource.c_str());
        
        glDeleteShader(m_shaderID);
        m_shaderID = 0;
        return false;
    }

    m_compiled = true;
    m_compileError.clear();
    return true;
}

void ShaderGLES::Destroy()
{
    if (m_shaderID != 0) {
        glDeleteShader(m_shaderID);
        m_shaderID = 0;
    }
    m_compiled = false;
}

bool ShaderGLES::IsCompiled() const
{
    return m_compiled;
}

const char* ShaderGLES::GetCompileError() const
{
    return m_compileError.c_str();
}

ShaderStage ShaderGLES::GetStage() const
{
    return m_stage;
}

ResourceHandle ShaderGLES::GetNativeHandle() const
{
    ResourceHandle handle;
    handle.glHandle = m_shaderID;
    return handle;
}

// ============================================================================
// ShaderProgramGLES
// ============================================================================

ShaderProgramGLES::ShaderProgramGLES()
    : m_programID(0)
    , m_linked(false)
{
}

ShaderProgramGLES::~ShaderProgramGLES()
{
    Destroy();
}

bool ShaderProgramGLES::Link(IShaderImpl** shaders, uint32_t count)
{
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
        
        ShaderGLES* glesShader = static_cast<ShaderGLES*>(shaders[i]);
        if (!glesShader->IsCompiled()) {
            m_linkError = "Attempting to link with uncompiled shader";
            LR_SET_ERROR(ErrorCode::ShaderLinkFailed, m_linkError.c_str());
            glDeleteProgram(m_programID);
            m_programID = 0;
            return false;
        }
        
        glAttachShader(m_programID, glesShader->GetShaderID());
    }

    // 链接程序
    glLinkProgram(m_programID);

    LR_LOG_DEBUG("OpenGL ES LinkProgram");

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
        LR_LOG_ERROR_F("OpenGL ES Shader Link Failed: %s", m_linkError.c_str());
        glDeleteProgram(m_programID);
        m_programID = 0;
        return false;
    }

    // 分离着色器（链接后不再需要）
    for (uint32_t i = 0; i < count; ++i) {
        if (shaders[i] == nullptr) {
            continue;
        }
        ShaderGLES* glesShader = static_cast<ShaderGLES*>(shaders[i]);
        glDetachShader(m_programID, glesShader->GetShaderID());
    }

    m_linked = true;
    m_linkError.clear();
    return true;
}

void ShaderProgramGLES::Destroy()
{
    if (m_programID != 0) {
        glDeleteProgram(m_programID);
        m_programID = 0;
    }
    m_linked = false;
}

bool ShaderProgramGLES::IsLinked() const
{
    return m_linked;
}

const char* ShaderProgramGLES::GetLinkError() const
{
    return m_linkError.c_str();
}

void ShaderProgramGLES::Use()
{
    if (m_programID != 0) {
        glUseProgram(m_programID);
        LR_LOG_DEBUG_F("OpenGL ES UseProgram: %d", m_programID);
    }
}

int32_t ShaderProgramGLES::GetUniformLocation(const char* name)
{
    if (m_programID == 0 || name == nullptr) {
        return -1;
    }
    return glGetUniformLocation(m_programID, name);
}

void ShaderProgramGLES::SetUniform1i(int32_t location, int32_t value)
{
    if (location >= 0) {
        glUniform1i(location, value);
    }
}

void ShaderProgramGLES::SetUniform1f(int32_t location, float value)
{
    if (location >= 0) {
        glUniform1f(location, value);
    }
}

void ShaderProgramGLES::SetUniform2f(int32_t location, float x, float y)
{
    if (location >= 0) {
        glUniform2f(location, x, y);
    }
}

void ShaderProgramGLES::SetUniform3f(int32_t location, float x, float y, float z)
{
    if (location >= 0) {
        glUniform3f(location, x, y, z);
    }
}

void ShaderProgramGLES::SetUniform4f(int32_t location, float x, float y, float z, float w)
{
    if (location >= 0) {
        glUniform4f(location, x, y, z, w);
    }
}

void ShaderProgramGLES::SetUniformMatrix3fv(int32_t location, const float* value, bool transpose)
{
    if (location >= 0 && value != nullptr) {
        glUniformMatrix3fv(location, 1, transpose ? GL_TRUE : GL_FALSE, value);
    }
}

void ShaderProgramGLES::SetUniformMatrix4fv(int32_t location, const float* value, bool transpose)
{
    if (location >= 0 && value != nullptr) {
        glUniformMatrix4fv(location, 1, transpose ? GL_TRUE : GL_FALSE, value);
    }
}

ResourceHandle ShaderProgramGLES::GetNativeHandle() const
{
    ResourceHandle handle;
    handle.glHandle = m_programID;
    return handle;
}

void ShaderProgramGLES::BindUniformBlock(const char* blockName, uint32_t bindingPoint)
{
    if (m_programID == 0 || blockName == nullptr) {
        return;
    }

    GLuint blockIndex = glGetUniformBlockIndex(m_programID, blockName);
    if (blockIndex != GL_INVALID_INDEX) {
        glUniformBlockBinding(m_programID, blockIndex, bindingPoint);
    }
}

} // namespace gles
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGLES
