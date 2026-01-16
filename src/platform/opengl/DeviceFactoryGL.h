/**
 * @file DeviceFactoryGL.h
 * @brief OpenGL设备工厂
 */

#pragma once

#include "lrengine/factory/LRDeviceFactory.h"

#ifdef LRENGINE_ENABLE_OPENGL

namespace lrengine {
namespace render {

/**
 * @brief OpenGL设备工厂
 */
class DeviceFactoryGL : public LRDeviceFactory {
public:
    DeviceFactoryGL() = default;
    ~DeviceFactoryGL() override = default;
    
    IRenderContextImpl* CreateRenderContextImpl() override;
    Backend GetBackend() const override { return Backend::OpenGL; }
    bool IsAvailable() const override;
};

} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGL
