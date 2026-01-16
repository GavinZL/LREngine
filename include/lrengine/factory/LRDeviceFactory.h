/**
 * @file LRDeviceFactory.h
 * @brief LREngine设备工厂
 */

#pragma once

#include "lrengine/core/LRDefines.h"
#include "lrengine/core/LRTypes.h"

namespace lrengine {
namespace render {

class IRenderContextImpl;

/**
 * @brief 设备工厂抽象基类
 * 
 * 负责创建特定后端的渲染上下文实现
 */
class LR_API LRDeviceFactory {
public:
    virtual ~LRDeviceFactory() = default;
    
    /**
     * @brief 创建渲染上下文实现
     * @return 上下文实现，失败返回nullptr
     */
    virtual IRenderContextImpl* CreateRenderContextImpl() = 0;
    
    /**
     * @brief 获取后端类型
     */
    virtual Backend GetBackend() const = 0;
    
    /**
     * @brief 检查后端是否可用
     */
    virtual bool IsAvailable() const = 0;
    
    /**
     * @brief 根据后端类型获取工厂实例
     * @param backend 后端类型
     * @return 工厂实例，不支持的后端返回nullptr
     */
    static LRDeviceFactory* GetFactory(Backend backend);
    
protected:
    LRDeviceFactory() = default;
};

} // namespace render
} // namespace lrengine
