/**
 * @file DeviceFactoryMTL.mm
 * @brief Metal设备工厂实现
 */

#include "DeviceFactoryMTL.h"

#ifdef LRENGINE_ENABLE_METAL

#include "ContextMTL.h"
#import <Metal/Metal.h>

namespace lrengine {
namespace render {

IRenderContextImpl* DeviceFactoryMTL::CreateRenderContextImpl() {
    return new mtl::RenderContextMTL();
}

bool DeviceFactoryMTL::IsAvailable() const {
    // 检查Metal是否可用
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    return device != nil;
}

} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
