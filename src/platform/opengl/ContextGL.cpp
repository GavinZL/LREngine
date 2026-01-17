/**
 * @file ContextGL.cpp
 * @brief OpenGL渲染上下文实现
 */

#include "ContextGL.h"

#ifdef LRENGINE_ENABLE_OPENGL

#include "BufferGL.h"
#include "ShaderGL.h"
#include "TextureGL.h"
#include "FrameBufferGL.h"
#include "PipelineStateGL.h"
#include "FenceGL.h"
#include "TypeConverterGL.h"
#include "lrengine/core/LRError.h"
#include "lrengine/utils/LRLog.h"

namespace lrengine {
namespace render {

RenderContextGL::RenderContextGL() = default;

RenderContextGL::~RenderContextGL() {
    Shutdown();
}

bool RenderContextGL::Initialize(const RenderContextDescriptor& desc) {
    mWindowHandle = desc.windowHandle;
    mWidth = desc.width;
    mHeight = desc.height;
    mDebug = desc.debug;
    
    // 注意：实际的OpenGL上下文创建需要平台特定代码（GLFW等）
    // 这里假设OpenGL上下文已经由外部创建并设置为当前
    
    // 创建默认VAO（OpenGL Core Profile需要）
    glGenVertexArrays(1, &mDefaultVAO);
    glBindVertexArray(mDefaultVAO);
    
    // 设置默认状态
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // 设置默认视口
    glViewport(0, 0, mWidth, mHeight);
    
    if (mDebug) {
        // 启用调试输出（如果支持）
        // glEnable(GL_DEBUG_OUTPUT);
    }
    
    return true;
}

void RenderContextGL::Shutdown() {
    if (mDefaultVAO) {
        glDeleteVertexArrays(1, &mDefaultVAO);
        mDefaultVAO = 0;
    }
}

void RenderContextGL::MakeCurrent() {
    // 需要平台特定实现
}

void RenderContextGL::SwapBuffers() {
    // 需要平台特定实现（如glfwSwapBuffers）
}

IBufferImpl* RenderContextGL::CreateBufferImpl(BufferType type) {
    switch (type) {
        case BufferType::Vertex:
            return new gl::VertexBufferGL();
        case BufferType::Index:
            return new gl::IndexBufferGL();
        case BufferType::Uniform:
            return new gl::UniformBufferGL();
        default:
            return new gl::BufferGL();
    }
}

IShaderImpl* RenderContextGL::CreateShaderImpl() {
    return new gl::ShaderGL();
}

IShaderProgramImpl* RenderContextGL::CreateShaderProgramImpl() {
    return new gl::ShaderProgramGL();
}

ITextureImpl* RenderContextGL::CreateTextureImpl() {
    return new gl::TextureGL();
}

IFrameBufferImpl* RenderContextGL::CreateFrameBufferImpl() {
    return new gl::FrameBufferGL();
}

IPipelineStateImpl* RenderContextGL::CreatePipelineStateImpl() {
    return new gl::PipelineStateGL();
}

IFenceImpl* RenderContextGL::CreateFenceImpl() {
    return new gl::FenceGL();
}

void RenderContextGL::SetViewport(int32_t x, int32_t y, int32_t width, int32_t height) {
    glViewport(x, y, width, height);
}

void RenderContextGL::SetScissor(int32_t x, int32_t y, int32_t width, int32_t height) {
    glScissor(x, y, width, height);
}

void RenderContextGL::Clear(uint8_t flags, const float* color, float depth, uint8_t stencil) {
    GLbitfield clearMask = 0;
    
    LR_LOG_TRACE_F("OpenGL Clear: flags=0x%x, depth=%.2f, stencil=%u", flags, depth, stencil);
    
    if (flags & ClearColor) {
        if (color) {
            glClearColor(color[0], color[1], color[2], color[3]);
        }
        clearMask |= GL_COLOR_BUFFER_BIT;
    }
    
    if (flags & ClearDepth) {
        glClearDepth(depth);
        clearMask |= GL_DEPTH_BUFFER_BIT;
    }
    
    if (flags & ClearStencil) {
        glClearStencil(stencil);
        clearMask |= GL_STENCIL_BUFFER_BIT;
    }
    
    if (clearMask) {
        glClear(clearMask);
    }
}

void RenderContextGL::BindPipelineState(IPipelineStateImpl* pipelineState) {
    LR_LOG_TRACE_F("OpenGL BindPipelineState: %p", pipelineState);
    if (pipelineState) {
        pipelineState->Apply();
    }
}

void RenderContextGL::BindVertexBuffer(IBufferImpl* buffer, uint32_t slot) {
    LR_LOG_TRACE_F("OpenGL BindVertexBuffer: %p, slot=%u", buffer, slot);
    if (buffer) {
        buffer->Bind();
    }
}

void RenderContextGL::BindIndexBuffer(IBufferImpl* buffer) {
    LR_LOG_TRACE_F("OpenGL BindIndexBuffer: %p", buffer);
    if (buffer) {
        buffer->Bind();
    }
}

void RenderContextGL::BindUniformBuffer(IBufferImpl* buffer, uint32_t slot) {
    LR_LOG_TRACE_F("OpenGL BindUniformBuffer: %p, slot=%u", buffer, slot);
    if (buffer) {
        static_cast<gl::UniformBufferGL*>(buffer)->BindToSlot(slot);
    }
}

void RenderContextGL::BindTexture(ITextureImpl* texture, uint32_t slot) {
    LR_LOG_TRACE_F("OpenGL BindTexture: %p, slot=%u", texture, slot);
    if (texture) {
        texture->Bind(slot);
    }
}

void RenderContextGL::DrawArrays(PrimitiveType primitiveType, uint32_t vertexStart, uint32_t vertexCount) {
    LR_LOG_TRACE_F("OpenGL DrawArrays: type=%d, start=%u, count=%u", (int)primitiveType, vertexStart, vertexCount);
    GLenum mode = gl::ToGLPrimitiveType(primitiveType);
    glDrawArrays(mode, vertexStart, vertexCount);
}

void RenderContextGL::DrawElements(PrimitiveType primitiveType, uint32_t indexCount,
                                  IndexType indexType, size_t indexOffset) {
    LR_LOG_TRACE_F("OpenGL DrawElements: type=%d, count=%u, indexType=%d, offset=%zu", (int)primitiveType, indexCount, (int)indexType, indexOffset);
    GLenum mode = gl::ToGLPrimitiveType(primitiveType);
    GLenum type = gl::ToGLIndexType(indexType);
    glDrawElements(mode, indexCount, type, reinterpret_cast<const void*>(indexOffset));
}

void RenderContextGL::DrawArraysInstanced(PrimitiveType primitiveType, uint32_t vertexStart,
                                         uint32_t vertexCount, uint32_t instanceCount) {
    GLenum mode = gl::ToGLPrimitiveType(primitiveType);
    glDrawArraysInstanced(mode, vertexStart, vertexCount, instanceCount);
}

void RenderContextGL::DrawElementsInstanced(PrimitiveType primitiveType, uint32_t indexCount,
                                           IndexType indexType, size_t indexOffset,
                                           uint32_t instanceCount) {
    GLenum mode = gl::ToGLPrimitiveType(primitiveType);
    GLenum type = gl::ToGLIndexType(indexType);
    glDrawElementsInstanced(mode, indexCount, type, 
                           reinterpret_cast<const void*>(indexOffset), instanceCount);
}

void RenderContextGL::WaitIdle() {
    glFinish();
}

void RenderContextGL::Flush() {
    glFlush();
}

} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGL
