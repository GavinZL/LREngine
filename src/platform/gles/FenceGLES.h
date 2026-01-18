/**
 * @file FenceGLES.h
 * @brief OpenGL ES同步栅栏实现
 */

#pragma once

#include "platform/interface/IFenceImpl.h"
#include "TypeConverterGLES.h"

#ifdef LRENGINE_ENABLE_OPENGLES

namespace lrengine {
namespace render {
namespace gles {

/**
 * @brief OpenGL ES同步栅栏实现
 */
class FenceGLES : public IFenceImpl {
public:
    FenceGLES();
    ~FenceGLES() override;

    // IFenceImpl接口
    bool Create() override;
    void Destroy() override;
    void Signal() override;
    bool Wait(uint64_t timeoutNs) override;
    FenceStatus GetStatus() const override;
    void Reset() override;
    ResourceHandle GetNativeHandle() const override;

private:
    GLsync m_sync;
    bool m_signaled;
};

} // namespace gles
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGLES
