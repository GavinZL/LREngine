/**
 * @file IPipelineStateImpl.h
 * @brief 管线状态平台实现接口
 */

#pragma once

#include "lrengine/core/LRTypes.h"

namespace lrengine {
namespace render {

class IShaderProgramImpl;

/**
 * @brief 管线状态实现接口
 */
class IPipelineStateImpl {
public:
    virtual ~IPipelineStateImpl() = default;
    
    /**
     * @brief 创建管线状态
     * @param desc 管线状态描述符
     * @return 成功返回true
     */
    virtual bool Create(const PipelineStateDescriptor& desc) = 0;
    
    /**
     * @brief 销毁管线状态
     */
    virtual void Destroy() = 0;
    
    /**
     * @brief 应用管线状态
     */
    virtual void Apply() = 0;
    
    /**
     * @brief 获取原生句柄
     */
    virtual ResourceHandle GetNativeHandle() const = 0;
    
    /**
     * @brief 获取图元类型
     */
    virtual PrimitiveType GetPrimitiveType() const = 0;
};

} // namespace render
} // namespace lrengine
