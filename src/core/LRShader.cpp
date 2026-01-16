/**
 * @file LRShader.cpp
 * @brief LREngine着色器实现
 */

#include "lrengine/core/LRShader.h"
#include "lrengine/core/LRError.h"
#include "platform/interface/IShaderImpl.h"
#include "lrengine/math/Vec2.hpp"
#include "lrengine/math/Vec3.hpp"
#include "lrengine/math/Vec4.hpp"
#include "lrengine/math/Mat3.hpp"
#include "lrengine/math/Mat4.hpp"

namespace lrengine {
namespace render {

// =============================================================================
// LRShader
// =============================================================================

LRShader::LRShader()
    : LRResource(ResourceType::Shader) {
}

LRShader::~LRShader() {
    if (mImpl) {
        mImpl->Destroy();
        delete mImpl;
        mImpl = nullptr;
    }
}

bool LRShader::Initialize(IShaderImpl* impl, const ShaderDescriptor& desc) {
    if (!impl) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Shader implementation is null");
        return false;
    }
    
    mImpl = impl;
    mStage = desc.stage;
    
    if (!mImpl->Compile(desc)) {
        mCompileError = mImpl->GetCompileError();
        LR_SET_ERROR(ErrorCode::ShaderCompileFailed, mCompileError.c_str());
        delete mImpl;
        mImpl = nullptr;
        return false;
    }
    
    mIsValid = true;
    
    if (desc.debugName) {
        SetDebugName(desc.debugName);
    }
    
    return true;
}

bool LRShader::IsCompiled() const {
    return mImpl && mImpl->IsCompiled();
}

const char* LRShader::GetCompileError() const {
    return mCompileError.c_str();
}

ResourceHandle LRShader::GetNativeHandle() const {
    if (mImpl) {
        return mImpl->GetNativeHandle();
    }
    return ResourceHandle();
}

// =============================================================================
// LRShaderProgram
// =============================================================================

LRShaderProgram::LRShaderProgram()
    : LRResource(ResourceType::Shader) {
}

LRShaderProgram::~LRShaderProgram() {
    if (mImpl) {
        mImpl->Destroy();
        delete mImpl;
        mImpl = nullptr;
    }
}

bool LRShaderProgram::Initialize(IShaderProgramImpl* impl,
                                LRShader* vertexShader,
                                LRShader* fragmentShader,
                                LRShader* geometryShader) {
    if (!impl) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Shader program implementation is null");
        return false;
    }
    
    if (!vertexShader || !vertexShader->IsCompiled()) {
        LR_SET_ERROR(ErrorCode::ShaderNotCompiled, "Vertex shader is not compiled");
        return false;
    }
    
    if (!fragmentShader || !fragmentShader->IsCompiled()) {
        LR_SET_ERROR(ErrorCode::ShaderNotCompiled, "Fragment shader is not compiled");
        return false;
    }
    
    mImpl = impl;
    mVertexShader = vertexShader;
    mFragmentShader = fragmentShader;
    mGeometryShader = geometryShader;
    
    // 收集着色器实现
    std::vector<IShaderImpl*> shaderImpls;
    shaderImpls.push_back(vertexShader->GetImpl());
    shaderImpls.push_back(fragmentShader->GetImpl());
    if (geometryShader && geometryShader->IsCompiled()) {
        shaderImpls.push_back(geometryShader->GetImpl());
    }
    
    if (!mImpl->Link(shaderImpls.data(), static_cast<uint32_t>(shaderImpls.size()))) {
        mLinkError = mImpl->GetLinkError();
        LR_SET_ERROR(ErrorCode::ShaderLinkFailed, mLinkError.c_str());
        delete mImpl;
        mImpl = nullptr;
        return false;
    }
    
    mIsValid = true;
    return true;
}

bool LRShaderProgram::IsLinked() const {
    return mImpl && mImpl->IsLinked();
}

const char* LRShaderProgram::GetLinkError() const {
    return mLinkError.c_str();
}

void LRShaderProgram::Use() {
    if (mImpl && mIsValid) {
        mImpl->Use();
    }
}

int32_t LRShaderProgram::GetUniformLocation(const char* name) {
    if (!mImpl || !mIsValid) {
        return -1;
    }
    
    // 检查缓存
    auto it = mUniformLocationCache.find(name);
    if (it != mUniformLocationCache.end()) {
        return it->second;
    }
    
    // 查询并缓存
    int32_t location = mImpl->GetUniformLocation(name);
    mUniformLocationCache[name] = location;
    return location;
}

void LRShaderProgram::SetUniform(const char* name, int32_t value) {
    int32_t location = GetUniformLocation(name);
    if (location >= 0 && mImpl) {
        mImpl->SetUniform1i(location, value);
    }
}

void LRShaderProgram::SetUniform(const char* name, float value) {
    int32_t location = GetUniformLocation(name);
    if (location >= 0 && mImpl) {
        mImpl->SetUniform1f(location, value);
    }
}

void LRShaderProgram::SetUniform(const char* name, float x, float y) {
    int32_t location = GetUniformLocation(name);
    if (location >= 0 && mImpl) {
        mImpl->SetUniform2f(location, x, y);
    }
}

void LRShaderProgram::SetUniform(const char* name, float x, float y, float z) {
    int32_t location = GetUniformLocation(name);
    if (location >= 0 && mImpl) {
        mImpl->SetUniform3f(location, x, y, z);
    }
}

void LRShaderProgram::SetUniform(const char* name, float x, float y, float z, float w) {
    int32_t location = GetUniformLocation(name);
    if (location >= 0 && mImpl) {
        mImpl->SetUniform4f(location, x, y, z, w);
    }
}

void LRShaderProgram::SetUniform(const char* name, const hyengine::math::Vec2& value) {
    SetUniform(name, value.x, value.y);
}

void LRShaderProgram::SetUniform(const char* name, const hyengine::math::Vec3& value) {
    SetUniform(name, value.x, value.y, value.z);
}

void LRShaderProgram::SetUniform(const char* name, const hyengine::math::Vec4& value) {
    SetUniform(name, value.x, value.y, value.z, value.w);
}

void LRShaderProgram::SetUniformMatrix3(const char* name, const float* value, bool transpose) {
    int32_t location = GetUniformLocation(name);
    if (location >= 0 && mImpl) {
        mImpl->SetUniformMatrix3fv(location, value, transpose);
    }
}

void LRShaderProgram::SetUniformMatrix4(const char* name, const float* value, bool transpose) {
    int32_t location = GetUniformLocation(name);
    if (location >= 0 && mImpl) {
        mImpl->SetUniformMatrix4fv(location, value, transpose);
    }
}

void LRShaderProgram::SetUniformMatrix3(const char* name, const hyengine::math::Mat3& value, bool transpose) {
    SetUniformMatrix3(name, value.m, transpose);
}

void LRShaderProgram::SetUniformMatrix4(const char* name, const hyengine::math::Mat4& value, bool transpose) {
    SetUniformMatrix4(name, value.m, transpose);
}

ResourceHandle LRShaderProgram::GetNativeHandle() const {
    if (mImpl) {
        return mImpl->GetNativeHandle();
    }
    return ResourceHandle();
}

} // namespace render
} // namespace lrengine
