/**
 * @file LRShader.h
 * @brief LREngine着色器对象
 */

#pragma once

#include "LRResource.h"
#include "LRTypes.h"

#include <unordered_map>
#include <string>
#include <vector>

namespace lrengine {
namespace render {

class IShaderImpl;
class IShaderProgramImpl;

/**
 * @brief 着色器类
 * 
 * 表示单个着色器阶段（顶点、片段等）
 */
class LR_API LRShader : public LRResource {
public:
    LR_NONCOPYABLE(LRShader);
    
    virtual ~LRShader();
    
    /**
     * @brief 检查是否已编译
     */
    bool IsCompiled() const;
    
    /**
     * @brief 获取编译错误信息
     */
    const char* GetCompileError() const;
    
    /**
     * @brief 获取着色器阶段
     */
    ShaderStage GetStage() const { return mStage; }
    
    /**
     * @brief 获取原生句柄
     */
    ResourceHandle GetNativeHandle() const override;
    
    /**
     * @brief 获取平台实现
     */
    IShaderImpl* GetImpl() const { return mImpl; }
    
protected:
    friend class LRRenderContext;
    
    LRShader();
    bool Initialize(IShaderImpl* impl, const ShaderDescriptor& desc);
    
protected:
    IShaderImpl* mImpl = nullptr;
    ShaderStage mStage = ShaderStage::Vertex;
    std::string mCompileError;
};

/**
 * @brief 着色器程序类
 * 
 * 链接多个着色器阶段形成完整的渲染程序
 */
class LR_API LRShaderProgram : public LRResource {
public:
    LR_NONCOPYABLE(LRShaderProgram);
    
    virtual ~LRShaderProgram();
    
    /**
     * @brief 检查是否已链接
     */
    bool IsLinked() const;
    
    /**
     * @brief 获取链接错误信息
     */
    const char* GetLinkError() const;
    
    /**
     * @brief 使用/绑定程序
     */
    void Use();
    
    /**
     * @brief 获取Uniform位置（带缓存）
     * @param name Uniform名称
     * @return 位置索引，-1表示未找到
     */
    int32_t GetUniformLocation(const char* name);
    
    // Uniform设置方法
    void SetUniform(const char* name, int32_t value);
    void SetUniform(const char* name, float value);
    void SetUniform(const char* name, float x, float y);
    void SetUniform(const char* name, float x, float y, float z);
    void SetUniform(const char* name, float x, float y, float z, float w);
    void SetUniform(const char* name, const hyengine::math::Vec2& value);
    void SetUniform(const char* name, const hyengine::math::Vec3& value);
    void SetUniform(const char* name, const hyengine::math::Vec4& value);
    void SetUniformMatrix3(const char* name, const float* value, bool transpose = false);
    void SetUniformMatrix4(const char* name, const float* value, bool transpose = false);
    void SetUniformMatrix3(const char* name, const hyengine::math::Mat3& value, bool transpose = false);
    void SetUniformMatrix4(const char* name, const hyengine::math::Mat4& value, bool transpose = false);
    
    /**
     * @brief 获取原生句柄
     */
    ResourceHandle GetNativeHandle() const override;
    
    /**
     * @brief 获取顶点着色器
     */
    LRShader* GetVertexShader() const { return mVertexShader; }
    
    /**
     * @brief 获取片段着色器
     */
    LRShader* GetFragmentShader() const { return mFragmentShader; }
    
protected:
    friend class LRRenderContext;
    
    LRShaderProgram();
    bool Initialize(IShaderProgramImpl* impl, 
                   LRShader* vertexShader, 
                   LRShader* fragmentShader,
                   LRShader* geometryShader = nullptr);
    
protected:
    IShaderProgramImpl* mImpl = nullptr;
    LRShader* mVertexShader = nullptr;
    LRShader* mFragmentShader = nullptr;
    LRShader* mGeometryShader = nullptr;
    std::string mLinkError;
    
    // Uniform位置缓存
    std::unordered_map<std::string, int32_t> mUniformLocationCache;
};

} // namespace render
} // namespace lrengine
