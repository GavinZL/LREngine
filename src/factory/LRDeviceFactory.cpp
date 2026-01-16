/**
 * @file LRDeviceFactory.cpp
 * @brief LREngine设备工厂实现
 */

#include "lrengine/factory/LRDeviceFactory.h"
#include "lrengine/core/LRError.h"

#ifdef LRENGINE_ENABLE_OPENGL
#include "platform/opengl/DeviceFactoryGL.h"
#endif

namespace lrengine {
namespace render {

LRDeviceFactory* LRDeviceFactory::GetFactory(Backend backend) {
    switch (backend) {
#ifdef LRENGINE_ENABLE_OPENGL
        case Backend::OpenGL: {
            static DeviceFactoryGL s_glFactory;
            if (s_glFactory.IsAvailable()) {
                return &s_glFactory;
            }
            break;
        }
#endif
        
#ifdef LRENGINE_ENABLE_OPENGLES
        case Backend::OpenGLES: {
            // TODO: 实现OpenGL ES工厂
            break;
        }
#endif
        
#ifdef LRENGINE_ENABLE_METAL
        case Backend::Metal: {
            // TODO: 实现Metal工厂
            break;
        }
#endif
        
#ifdef LRENGINE_ENABLE_VULKAN
        case Backend::Vulkan: {
            // TODO: 实现Vulkan工厂
            break;
        }
#endif
        
        default:
            break;
    }
    
    LR_SET_ERROR(ErrorCode::BackendNotAvailable, "Requested backend is not available");
    return nullptr;
}

} // namespace render
} // namespace lrengine
