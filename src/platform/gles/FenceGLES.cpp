/**
 * @file FenceGLES.cpp
 * @brief OpenGL ES同步栅栏实现
 */

#include "FenceGLES.h"
#include "lrengine/core/LRError.h"
#include "lrengine/utils/LRLog.h"

#ifdef LRENGINE_ENABLE_OPENGLES

namespace lrengine {
namespace render {
namespace gles {

FenceGLES::FenceGLES() : m_sync(nullptr), m_signaled(false) {}

FenceGLES::~FenceGLES() { Destroy(); }

bool FenceGLES::Create() {
    // OpenGL ES的同步对象通过Signal创建
    m_signaled = false;
    return true;
}

void FenceGLES::Destroy() {
    if (m_sync != nullptr) {
        glDeleteSync(m_sync);
        m_sync = nullptr;
    }
    m_signaled = false;
}

void FenceGLES::Signal() {
    // 如果已有同步对象，先删除
    if (m_sync != nullptr) {
        glDeleteSync(m_sync);
    }

    // 创建新的同步对象
    // GL_SYNC_GPU_COMMANDS_COMPLETE表示等待所有GPU命令完成
    m_sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    if (m_sync == nullptr) {
        LR_LOG_ERROR("Failed to create OpenGL ES fence sync object");
        m_signaled = false;
    } else {
        m_signaled = false; // 将在Wait中设置为true
        LR_LOG_TRACE("OpenGL ES fence sync created");
    }
}

bool FenceGLES::Wait(uint64_t timeoutNs) {
    if (m_sync == nullptr) {
        return true; // 没有同步对象，认为已完成
    }

    // 等待同步对象
    // GL_SYNC_FLUSH_COMMANDS_BIT确保命令被刷新到GPU
    GLenum result = glClientWaitSync(m_sync, GL_SYNC_FLUSH_COMMANDS_BIT, timeoutNs);

    switch (result) {
        case GL_ALREADY_SIGNALED:
        case GL_CONDITION_SATISFIED:
            m_signaled = true;
            return true;

        case GL_TIMEOUT_EXPIRED:
            return false;

        case GL_WAIT_FAILED:
        default:
            LR_LOG_ERROR("OpenGL ES fence wait failed");
            return false;
    }
}

FenceStatus FenceGLES::GetStatus() const {
    if (m_sync == nullptr) {
        return FenceStatus::Unsignaled;
    }

    GLint status;
    GLsizei length;
    glGetSynciv(m_sync, GL_SYNC_STATUS, sizeof(status), &length, &status);

    if (status == GL_SIGNALED) {
        return FenceStatus::Signaled;
    }

    return FenceStatus::Unsignaled;
}

void FenceGLES::Reset() {
    // OpenGL ES的同步对象不能重置，只能删除并创建新的
    Destroy();
    Create();
}

ResourceHandle FenceGLES::GetNativeHandle() const {
    ResourceHandle handle;
    handle.ptr = m_sync;
    return handle;
}

} // namespace gles
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGLES
