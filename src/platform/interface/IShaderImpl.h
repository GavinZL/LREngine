/**
 * @file IShaderImpl.h
 * @brief 着色器平台实现接口
 */

#pragma once

#include "lrengine/core/LRTypes.h"

namespace lrengine {
namespace render {

/**
 * @brief 着色器实现接口
 */
class IShaderImpl {
public:
    virtual ~IShaderImpl() = default;
    
    /**
     * @brief 编译着色器
     * @param desc 着色器描述符
     * @return 成功返回true
     */
    virtual bool Compile(const ShaderDescriptor& desc) = 0;
    
    /**
     * @brief 销毁着色器
     */
    virtual void Destroy() = 0;
    
    /**
     * @brief 检查是否已编译
     */
    virtual bool IsCompiled() const = 0;
    
    /**
     * @brief 获取编译错误信息
     */
    virtual const char* GetCompileError() const = 0;
    
    /**
     * @brief 获取着色器阶段
     */
    virtual ShaderStage GetStage() const = 0;
    
    /**
     * @brief 获取原生句柄
     */
    virtual ResourceHandle GetNativeHandle() const = 0;
};

/**
 * @brief 着色器程序实现接口
 */
class IShaderProgramImpl {
public:
    virtual ~IShaderProgramImpl() = default;
    
    /**
     * @brief 链接着色器程序
     * @param shaders 着色器数组
     * @param count 着色器数量
     * @return 成功返回true
     */
    virtual bool Link(IShaderImpl** shaders, uint32_t count) = 0;
    
    /**
     * @brief 销毁程序
     */
    virtual void Destroy() = 0;
    
    /**
     * @brief 检查是否已链接
     */
    virtual bool IsLinked() const = 0;
    
    /**
     * @brief 获取链接错误信息
     */
    virtual const char* GetLinkError() const = 0;
    
    /**
     * @brief 使用/绑定程序
     */
    virtual void Use() = 0;
    
    /**
     * @brief 获取Uniform位置
     * @param name Uniform名称
     * @return 位置索引，-1表示未找到
     */
    virtual int32_t GetUniformLocation(const char* name) = 0;
    
    /**
     * @brief 设置Uniform值
     */
    virtual void SetUniform1i(int32_t location, int32_t value) = 0;
    virtual void SetUniform1f(int32_t location, float value) = 0;
    virtual void SetUniform2f(int32_t location, float x, float y) = 0;
    virtual void SetUniform3f(int32_t location, float x, float y, float z) = 0;
    virtual void SetUniform4f(int32_t location, float x, float y, float z, float w) = 0;
    virtual void SetUniformMatrix3fv(int32_t location, const float* value, bool transpose = false) = 0;
    virtual void SetUniformMatrix4fv(int32_t location, const float* value, bool transpose = false) = 0;
    
    /**
     * @brief 获取原生句柄
     */
    virtual ResourceHandle GetNativeHandle() const = 0;
};

} // namespace render
} // namespace lrengine
