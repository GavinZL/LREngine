/**
 * @file FenceGL.h
 * @brief OpenGL同步屏障实现
 */

#pragma once

#include "platform/interface/IFenceImpl.h"
#include "TypeConverterGL.h"

#ifdef LRENGINE_ENABLE_OPENGL

namespace lrengine {
namespace render {
namespace gl {

/**
 * @brief OpenGL同步屏障实现
 * 
 * 使用glFenceSync实现GPU-CPU同步
 */
class FenceGL : public IFenceImpl {
public:
    FenceGL();
    ~FenceGL() override;

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

/**
 * @brief OpenGL查询对象封装
 */
class QueryGL {
public:
    enum class Type {
        SamplesPassed,
        AnySamplesPassed,
        TimeElapsed,
        PrimitivesGenerated,
        TransformFeedbackWritten
    };

    QueryGL();
    ~QueryGL();

    bool Create(Type type);
    void Destroy();
    void Begin();
    void End();
    bool IsResultAvailable() const;
    uint64_t GetResult() const;

    GLuint GetQueryID() const { return m_queryID; }

private:
    GLuint m_queryID;
    GLenum m_target;
    bool m_active;
};

} // namespace gl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGL
