/**
 * @file ShaderMTL.mm
 * @brief Metal着色器实现
 */

#include "ShaderMTL.h"

#ifdef LRENGINE_ENABLE_METAL

#include "lrengine/core/LRError.h"
#include <cstring>

namespace lrengine {
namespace render {
namespace mtl {

// =============================================================================
// ShaderMTL
// =============================================================================

ShaderMTL::ShaderMTL(id<MTLDevice> device)
    : m_device(device)
    , m_library(nil)
    , m_function(nil)
    , m_stage(ShaderStage::Vertex)
    , m_compiled(false)
    , m_entryPoint("main")
{
}

ShaderMTL::~ShaderMTL() {
    Destroy();
}

bool ShaderMTL::Compile(const ShaderDescriptor& desc) {
    if (!desc.source) {
        m_compileError = "Shader source is null";
        LR_SET_ERROR(ErrorCode::InvalidArgument, m_compileError.c_str());
        return false;
    }

    m_stage = desc.stage;
    m_entryPoint = desc.entryPoint ? desc.entryPoint : "main";

    // Metal只接受MSL (Metal Shading Language)
    if (desc.language != ShaderLanguage::MSL) {
        m_compileError = "Metal backend only supports MSL shaders. Please provide Metal Shading Language source.";
        LR_SET_ERROR(ErrorCode::InvalidArgument, m_compileError.c_str());
        return false;
    }

    NSString* source = [NSString stringWithUTF8String:desc.source];
    
    NSError* error = nil;
    MTLCompileOptions* options = [[MTLCompileOptions alloc] init];
    // 使用mathMode代替已弃用的fastMathEnabled
    if (@available(macOS 15.0, iOS 18.0, *)) {
        options.mathMode = MTLMathModeFast;
    } else {
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wdeprecated-declarations"
        options.fastMathEnabled = YES;
        #pragma clang diagnostic pop
    }
    
    m_library = [m_device newLibraryWithSource:source
                                       options:options
                                         error:&error];
    
    if (error || !m_library) {
        if (error) {
            m_compileError = [[error localizedDescription] UTF8String];
        } else {
            m_compileError = "Unknown error compiling Metal shader";
        }
        LR_SET_ERROR(ErrorCode::ShaderCompileFailed, m_compileError.c_str());
        return false;
    }

    // 获取函数
    NSString* entryPoint = [NSString stringWithUTF8String:m_entryPoint.c_str()];
    m_function = [m_library newFunctionWithName:entryPoint];
    
    if (!m_function) {
        m_compileError = "Failed to find function '" + m_entryPoint + "' in Metal shader";
        LR_SET_ERROR(ErrorCode::ShaderCompileFailed, m_compileError.c_str());
        return false;
    }

    if (desc.debugName) {
        m_function.label = [NSString stringWithUTF8String:desc.debugName];
    }

    m_compiled = true;
    return true;
}

void ShaderMTL::Destroy() {
    m_function = nil;
    m_library = nil;
    m_compiled = false;
    m_compileError.clear();
}

bool ShaderMTL::IsCompiled() const {
    return m_compiled;
}

const char* ShaderMTL::GetCompileError() const {
    return m_compileError.c_str();
}

ShaderStage ShaderMTL::GetStage() const {
    return m_stage;
}

ResourceHandle ShaderMTL::GetNativeHandle() const {
    return ResourceHandle((__bridge void*)m_function);
}

// =============================================================================
// ShaderProgramMTL
// =============================================================================

ShaderProgramMTL::ShaderProgramMTL(id<MTLDevice> device)
    : m_device(device)
    , m_vertexFunction(nil)
    , m_fragmentFunction(nil)
    , m_uniformBuffer(nil)
    , m_linked(false)
    , m_uniformsDirty(false)
    , m_nextUniformLocation(0)
{
}

ShaderProgramMTL::~ShaderProgramMTL() {
    Destroy();
}

bool ShaderProgramMTL::Link(IShaderImpl** shaders, uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        ShaderMTL* shader = static_cast<ShaderMTL*>(shaders[i]);
        if (!shader || !shader->IsCompiled()) {
            m_linkError = "Invalid or uncompiled shader provided";
            LR_SET_ERROR(ErrorCode::ShaderLinkFailed, m_linkError.c_str());
            return false;
        }

        switch (shader->GetStage()) {
            case ShaderStage::Vertex:
                m_vertexFunction = shader->GetFunction();
                break;
            case ShaderStage::Fragment:
                m_fragmentFunction = shader->GetFunction();
                break;
            default:
                // 其他着色器阶段暂不支持
                break;
        }
    }

    if (!m_vertexFunction) {
        m_linkError = "No vertex shader provided";
        LR_SET_ERROR(ErrorCode::ShaderLinkFailed, m_linkError.c_str());
        return false;
    }

    // 创建Uniform缓冲区
    CreateUniformBuffer();

    m_linked = true;
    return true;
}

void ShaderProgramMTL::Destroy() {
    m_vertexFunction = nil;
    m_fragmentFunction = nil;
    m_uniformBuffer = nil;
    m_linked = false;
    m_linkError.clear();
    m_uniformLocations.clear();
    m_uniformData.clear();
}

bool ShaderProgramMTL::IsLinked() const {
    return m_linked;
}

const char* ShaderProgramMTL::GetLinkError() const {
    return m_linkError.c_str();
}

void ShaderProgramMTL::Use() {
    // Metal中程序绑定在创建渲染管线状态时完成
    // 这里更新Uniform数据
    if (m_uniformsDirty) {
        UpdateUniforms();
    }
}

int32_t ShaderProgramMTL::GetUniformLocation(const char* name) {
    if (!name) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Uniform name is null");
        return -1;
    }

    std::string uniformName(name);
    
    auto it = m_uniformLocations.find(uniformName);
    if (it != m_uniformLocations.end()) {
        return static_cast<int32_t>(it->second.offset);
    }

    // 为新的uniform分配位置
    // 在Metal中，我们使用uniform缓冲区，所以这里分配的是偏移量
    int32_t location = m_nextUniformLocation++;
    
    // 注册一个默认大小（最大为mat4）
    UniformData data;
    data.offset = m_uniformData.size();
    data.size = 64; // sizeof(mat4)
    m_uniformLocations[uniformName] = data;
    
    // 扩展uniform数据缓冲区
    m_uniformData.resize(m_uniformData.size() + data.size, 0);
    
    return location;
}

void ShaderProgramMTL::SetUniform1i(int32_t location, int32_t value) {
    if (location < 0 || static_cast<size_t>(location) >= m_uniformData.size()) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Invalid uniform location");
        return;
    }
    
    memcpy(&m_uniformData[location], &value, sizeof(value));
    m_uniformsDirty = true;
}

void ShaderProgramMTL::SetUniform1f(int32_t location, float value) {
    if (location < 0 || static_cast<size_t>(location) >= m_uniformData.size()) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Invalid uniform location");
        return;
    }
    
    memcpy(&m_uniformData[location], &value, sizeof(value));
    m_uniformsDirty = true;
}

void ShaderProgramMTL::SetUniform2f(int32_t location, float x, float y) {
    if (location < 0 || static_cast<size_t>(location + 8) > m_uniformData.size()) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Invalid uniform location");
        return;
    }
    
    float data[2] = {x, y};
    memcpy(&m_uniformData[location], data, sizeof(data));
    m_uniformsDirty = true;
}

void ShaderProgramMTL::SetUniform3f(int32_t location, float x, float y, float z) {
    if (location < 0 || static_cast<size_t>(location + 12) > m_uniformData.size()) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Invalid uniform location");
        return;
    }
    
    float data[3] = {x, y, z};
    memcpy(&m_uniformData[location], data, sizeof(data));
    m_uniformsDirty = true;
}

void ShaderProgramMTL::SetUniform4f(int32_t location, float x, float y, float z, float w) {
    if (location < 0 || static_cast<size_t>(location + 16) > m_uniformData.size()) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Invalid uniform location");
        return;
    }
    
    float data[4] = {x, y, z, w};
    memcpy(&m_uniformData[location], data, sizeof(data));
    m_uniformsDirty = true;
}

void ShaderProgramMTL::SetUniformMatrix3fv(int32_t location, const float* value, bool transpose) {
    LR_UNUSED(transpose);
    
    if (location < 0 || !value || static_cast<size_t>(location + 36) > m_uniformData.size()) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Invalid uniform location or value");
        return;
    }
    
    memcpy(&m_uniformData[location], value, 9 * sizeof(float));
    m_uniformsDirty = true;
}

void ShaderProgramMTL::SetUniformMatrix4fv(int32_t location, const float* value, bool transpose) {
    LR_UNUSED(transpose);
    
    if (location < 0 || !value || static_cast<size_t>(location + 64) > m_uniformData.size()) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Invalid uniform location or value");
        return;
    }
    
    memcpy(&m_uniformData[location], value, 16 * sizeof(float));
    m_uniformsDirty = true;
}

ResourceHandle ShaderProgramMTL::GetNativeHandle() const {
    return ResourceHandle((__bridge void*)m_vertexFunction);
}

void ShaderProgramMTL::CreateUniformBuffer() {
    // 初始分配一个适当大小的uniform缓冲区
    const size_t initialSize = 4096; // 4KB
    m_uniformData.resize(initialSize, 0);
    
    m_uniformBuffer = [m_device newBufferWithLength:initialSize
                                            options:MTLResourceStorageModeShared];
    
    if (m_uniformBuffer) {
        m_uniformBuffer.label = @"ShaderProgram Uniform Buffer";
    }
}

void ShaderProgramMTL::UpdateUniforms() {
    if (!m_uniformBuffer || m_uniformData.empty()) {
        LR_SET_ERROR(ErrorCode::InvalidState, "Invalid uniform buffer or data");
        return;
    }
    
    void* contents = [m_uniformBuffer contents];
    if (contents) {
        memcpy(contents, m_uniformData.data(), m_uniformData.size());
    }
    
    m_uniformsDirty = false;
}

} // namespace mtl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
