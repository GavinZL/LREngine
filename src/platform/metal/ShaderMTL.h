/**
 * @file ShaderMTL.h
 * @brief Metal着色器实现
 */

#pragma once

#include "platform/interface/IShaderImpl.h"
#include <string>
#include <unordered_map>

#ifdef LRENGINE_ENABLE_METAL

#import <Metal/Metal.h>

namespace lrengine {
namespace render {
namespace mtl {

/**
 * @brief Metal着色器实现
 * 
 * 注意：Metal不支持单独的着色器对象，
 * 这个类主要用于存储编译后的着色器函数
 */
class ShaderMTL : public IShaderImpl {
public:
    ShaderMTL(id<MTLDevice> device);
    ~ShaderMTL() override;

    // IShaderImpl接口
    bool Compile(const ShaderDescriptor& desc) override;
    void Destroy() override;
    bool IsCompiled() const override;
    const char* GetCompileError() const override;
    ShaderStage GetStage() const override;
    ResourceHandle GetNativeHandle() const override;

    // Metal特有方法
    id<MTLFunction> GetFunction() const { return m_function; }
    id<MTLLibrary> GetLibrary() const { return m_library; }
    const std::string& GetEntryPoint() const { return m_entryPoint; }

private:
    id<MTLDevice> m_device;
    id<MTLLibrary> m_library;
    id<MTLFunction> m_function;
    ShaderStage m_stage;
    bool m_compiled;
    std::string m_compileError;
    std::string m_entryPoint;
};

/**
 * @brief Metal着色器程序实现
 * 
 * 管理顶点和片段着色器函数
 */
class ShaderProgramMTL : public IShaderProgramImpl {
public:
    ShaderProgramMTL(id<MTLDevice> device);
    ~ShaderProgramMTL() override;

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

    // Metal特有方法
    id<MTLFunction> GetVertexFunction() const { return m_vertexFunction; }
    id<MTLFunction> GetFragmentFunction() const { return m_fragmentFunction; }

    // Uniform缓冲区管理
    id<MTLBuffer> GetUniformBuffer() const { return m_uniformBuffer; }
    void UpdateUniforms();

private:
    void CreateUniformBuffer();

    id<MTLDevice> m_device;
    id<MTLFunction> m_vertexFunction;
    id<MTLFunction> m_fragmentFunction;
    id<MTLBuffer> m_uniformBuffer;

    bool m_linked;
    std::string m_linkError;

    // Uniform数据存储
    struct UniformData {
        size_t offset;
        size_t size;
    };
    std::unordered_map<std::string, UniformData> m_uniformLocations;
    std::vector<uint8_t> m_uniformData;
    bool m_uniformsDirty;
    int32_t m_nextUniformLocation;
};

} // namespace mtl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
