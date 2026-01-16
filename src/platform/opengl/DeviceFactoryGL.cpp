/**
 * @file DeviceFactoryGL.cpp
 * @brief OpenGL设备工厂实现
 */

#include "DeviceFactoryGL.h"

#ifdef LRENGINE_ENABLE_OPENGL

#include "ContextGL.h"

namespace lrengine {
namespace render {

IRenderContextImpl* DeviceFactoryGL::CreateRenderContextImpl() {
    return new RenderContextGL();
}

bool DeviceFactoryGL::IsAvailable() const {
    // OpenGL在大多数平台上都可用
    return true;
}

} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGL
