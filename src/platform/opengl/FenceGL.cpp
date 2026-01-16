/**
 * @file FenceGL.cpp
 * @brief OpenGL同步屏障实现
 */

#include "FenceGL.h"
#include "lrengine/core/LRError.h"

#ifdef LRENGINE_ENABLE_OPENGL

namespace lrengine {
namespace render {
namespace gl {

// ============================================================================
// FenceGL
// ============================================================================

FenceGL::FenceGL()
    : m_sync(nullptr)
    , m_signaled(false)
{
}

FenceGL::~FenceGL()
{
    Destroy();
}

bool FenceGL::Create()
{
    if (m_sync != nullptr) {
        Destroy();
    }
    m_signaled = false;
    return true;
}

void FenceGL::Destroy()
{
    if (m_sync != nullptr) {
        glDeleteSync(m_sync);
        m_sync = nullptr;
    }
    m_signaled = false;
}

void FenceGL::Signal()
{
    if (m_sync != nullptr) {
        glDeleteSync(m_sync);
    }
    
    m_sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    if (m_sync == nullptr) {
        LR_SET_ERROR(ErrorCode::FenceError, "Failed to create fence sync");
    }
    m_signaled = false;
}

bool FenceGL::Wait(uint64_t timeoutNs)
{
    if (m_sync == nullptr) {
        return true; // 没有同步对象，立即返回
    }

    if (m_signaled) {
        return true;
    }

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
            LR_SET_ERROR(ErrorCode::FenceError, "Fence wait failed");
            return false;
    }
}

FenceStatus FenceGL::GetStatus() const
{
    if (m_sync == nullptr) {
        return FenceStatus::Unsignaled;
    }

    if (m_signaled) {
        return FenceStatus::Signaled;
    }

    GLint status;
    GLsizei length;
    glGetSynciv(m_sync, GL_SYNC_STATUS, sizeof(status), &length, &status);

    if (status == GL_SIGNALED) {
        return FenceStatus::Signaled;
    }
    return FenceStatus::Unsignaled;
}

void FenceGL::Reset()
{
    if (m_sync != nullptr) {
        glDeleteSync(m_sync);
        m_sync = nullptr;
    }
    m_signaled = false;
}

ResourceHandle FenceGL::GetNativeHandle() const
{
    ResourceHandle handle;
    handle.ptr = m_sync;
    return handle;
}

// ============================================================================
// QueryGL
// ============================================================================

QueryGL::QueryGL()
    : m_queryID(0)
    , m_target(GL_SAMPLES_PASSED)
    , m_active(false)
{
}

QueryGL::~QueryGL()
{
    Destroy();
}

bool QueryGL::Create(Type type)
{
    if (m_queryID != 0) {
        Destroy();
    }

    switch (type) {
        case Type::SamplesPassed:
            m_target = GL_SAMPLES_PASSED;
            break;
        case Type::AnySamplesPassed:
            m_target = GL_ANY_SAMPLES_PASSED;
            break;
        case Type::TimeElapsed:
            m_target = GL_TIME_ELAPSED;
            break;
        case Type::PrimitivesGenerated:
            m_target = GL_PRIMITIVES_GENERATED;
            break;
        case Type::TransformFeedbackWritten:
            m_target = GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN;
            break;
        default:
            return false;
    }

    glGenQueries(1, &m_queryID);
    return m_queryID != 0;
}

void QueryGL::Destroy()
{
    if (m_queryID != 0) {
        if (m_active) {
            End();
        }
        glDeleteQueries(1, &m_queryID);
        m_queryID = 0;
    }
}

void QueryGL::Begin()
{
    if (m_queryID != 0 && !m_active) {
        glBeginQuery(m_target, m_queryID);
        m_active = true;
    }
}

void QueryGL::End()
{
    if (m_queryID != 0 && m_active) {
        glEndQuery(m_target);
        m_active = false;
    }
}

bool QueryGL::IsResultAvailable() const
{
    if (m_queryID == 0 || m_active) {
        return false;
    }

    GLint available;
    glGetQueryObjectiv(m_queryID, GL_QUERY_RESULT_AVAILABLE, &available);
    return available == GL_TRUE;
}

uint64_t QueryGL::GetResult() const
{
    if (m_queryID == 0) {
        return 0;
    }

    GLuint64 result;
    glGetQueryObjectui64v(m_queryID, GL_QUERY_RESULT, &result);
    return result;
}

} // namespace gl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGL
