/**
 * @file DeviceFactoryGLES.cpp
 * @brief OpenGL ES设备工厂实现
 */

#include "DeviceFactoryGLES.h"
#include "ContextGLES.h"
#include "lrengine/utils/LRLog.h"

#ifdef LRENGINE_ENABLE_OPENGLES

namespace lrengine {
namespace render {

IRenderContextImpl* DeviceFactoryGLES::CreateRenderContextImpl() {
    LR_LOG_INFO("Creating OpenGL ES render context");
    return new RenderContextGLES();
}

bool DeviceFactoryGLES::IsAvailable() const {
    // OpenGL ES可用性检查
    // 在移动平台上，如果编译了GLES后端，通常就是可用的
    // 实际可用性在上下文创建时验证
#if defined(__ANDROID__) || (defined(__APPLE__) && (TARGET_OS_IPHONE || TARGET_OS_SIMULATOR))
    return true;
#elif defined(__EMSCRIPTEN__)
    return true; // WebGL
#else
    // 在其他平台上可能通过EGL支持
    return false;
#endif
}

} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGLES
