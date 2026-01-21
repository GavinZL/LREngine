/**
 * @file DeviceFactoryMTL.h
 * @brief Metal设备工厂
 */

#pragma once

#include "lrengine/factory/LRDeviceFactory.h"

#ifdef LRENGINE_ENABLE_METAL

namespace lrengine {
namespace render {

/**
 * @brief Metal设备工厂
 */
class DeviceFactoryMTL : public LRDeviceFactory {
public:
    DeviceFactoryMTL()           = default;
    ~DeviceFactoryMTL() override = default;

    IRenderContextImpl* CreateRenderContextImpl() override;
    Backend GetBackend() const override { return Backend::Metal; }
    bool IsAvailable() const override;
};

} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
