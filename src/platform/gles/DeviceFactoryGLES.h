/**
 * @file DeviceFactoryGLES.h
 * @brief OpenGL ES设备工厂
 */

#pragma once

#include "lrengine/factory/LRDeviceFactory.h"

#ifdef LRENGINE_ENABLE_OPENGLES

namespace lrengine {
namespace render {

/**
 * @brief OpenGL ES设备工厂
 */
class DeviceFactoryGLES : public LRDeviceFactory {
public:
    DeviceFactoryGLES()           = default;
    ~DeviceFactoryGLES() override = default;

    /**
     * @brief 创建OpenGL ES渲染上下文实现
     */
    IRenderContextImpl* CreateRenderContextImpl() override;

    /**
     * @brief 获取后端类型
     */
    Backend GetBackend() const override { return Backend::OpenGLES; }

    /**
     * @brief 检查OpenGL ES是否可用
     */
    bool IsAvailable() const override;
};

} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGLES
