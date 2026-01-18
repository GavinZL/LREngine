/**
 * @file ContextGLES.cpp
 * @brief OpenGL ES渲染上下文实现
 */

#include "ContextGLES.h"

#ifdef LRENGINE_ENABLE_OPENGLES

#include "BufferGLES.h"
#include "ShaderGLES.h"
#include "TextureGLES.h"
#include "FrameBufferGLES.h"
#include "PipelineStateGLES.h"
#include "FenceGLES.h"
#include "TypeConverterGLES.h"
#include "lrengine/core/LRError.h"
#include "lrengine/utils/LRLog.h"

namespace lrengine {
namespace render {

RenderContextGLES::RenderContextGLES() = default;

RenderContextGLES::~RenderContextGLES() {
    Shutdown();
}

bool RenderContextGLES::Initialize(const RenderContextDescriptor& desc) {
    m_windowHandle = desc.windowHandle;
    m_width = desc.width;
    m_height = desc.height;
    m_debug = desc.debug;
    
    // 注意：实际的OpenGL ES上下文创建需要平台特定代码（EGL等）
    // 这里假设上下文已经由外部创建并设置为当前
    
    // 初始化能力检测
    m_capabilities.Initialize();
    
    // 创建默认VAO（OpenGL ES 3.0需要VAO）
    glGenVertexArrays(1, &m_defaultVAO);
    
    // 设置默认状态
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // 设置默认视口
    glViewport(0, 0, m_width, m_height);
    
    // 调试输出
    if (m_debug && m_capabilities.hasDebugOutput) {
#ifdef GL_KHR_debug
        glEnable(GL_DEBUG_OUTPUT_KHR);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR);
#endif
    }
    
    LR_LOG_INFO_F("OpenGL ES context initialized: %dx%d", m_width, m_height);
    return true;
}

void RenderContextGLES::Shutdown() {
    if (m_defaultVAO) {
        glDeleteVertexArrays(1, &m_defaultVAO);
        m_defaultVAO = 0;
    }
    
    LR_LOG_INFO("OpenGL ES context shutdown");
}

void RenderContextGLES::MakeCurrent() {
    // 需要平台特定实现（如eglMakeCurrent）
}

void RenderContextGLES::SwapBuffers() {
    // 需要平台特定实现（如eglSwapBuffers）
}

void RenderContextGLES::BeginFrame() {
    // OpenGL ES不需要特殊的帧开始处理
}

void RenderContextGLES::EndFrame() {
    // OpenGL ES不需要特殊的帧结束处理
}

IBufferImpl* RenderContextGLES::CreateBufferImpl(BufferType type) {
    switch (type) {
        case BufferType::Vertex:
            return new gles::VertexBufferGLES();
        case BufferType::Index:
            return new gles::IndexBufferGLES();
        case BufferType::Uniform:
            return new gles::UniformBufferGLES();
        default:
            return new gles::BufferGLES();
    }
}

IShaderImpl* RenderContextGLES::CreateShaderImpl() {
    return new gles::ShaderGLES();
}

IShaderProgramImpl* RenderContextGLES::CreateShaderProgramImpl() {
    return new gles::ShaderProgramGLES();
}

ITextureImpl* RenderContextGLES::CreateTextureImpl() {
    return new gles::TextureGLES();
}

IFrameBufferImpl* RenderContextGLES::CreateFrameBufferImpl() {
    return new gles::FrameBufferGLES();
}

IPipelineStateImpl* RenderContextGLES::CreatePipelineStateImpl() {
    return new gles::PipelineStateGLES();
}

IFenceImpl* RenderContextGLES::CreateFenceImpl() {
    return new gles::FenceGLES();
}

void RenderContextGLES::SetViewport(int32_t x, int32_t y, int32_t width, int32_t height) {
    glViewport(x, y, width, height);
}

void RenderContextGLES::SetScissor(int32_t x, int32_t y, int32_t width, int32_t height) {
    glScissor(x, y, width, height);
}

void RenderContextGLES::Clear(uint8_t flags, const float* color, float depth, uint8_t stencil) {
    GLbitfield clearMask = 0;
    
    LR_LOG_TRACE_F("OpenGL ES Clear: flags=0x%x, depth=%.2f, stencil=%u", flags, depth, stencil);
    
    if (flags & ClearColor) {
        if (color) {
            glClearColor(color[0], color[1], color[2], color[3]);
        }
        clearMask |= GL_COLOR_BUFFER_BIT;
    }
    
    if (flags & ClearDepth) {
        // OpenGL ES使用glClearDepthf而不是glClearDepth
        glClearDepthf(depth);
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

void RenderContextGLES::BindPipelineState(IPipelineStateImpl* pipelineState) {
    LR_LOG_TRACE_F("OpenGL ES BindPipelineState: %p", pipelineState);
    if (pipelineState) {
        pipelineState->Apply();
    }
}

void RenderContextGLES::BindVertexBuffer(IBufferImpl* buffer, uint32_t slot) {
    LR_LOG_TRACE_F("OpenGL ES BindVertexBuffer: %p, slot=%u", buffer, slot);
    if (buffer) {
        buffer->Bind();
    }
}

void RenderContextGLES::BindIndexBuffer(IBufferImpl* buffer) {
    LR_LOG_TRACE_F("OpenGL ES BindIndexBuffer: %p", buffer);
    if (buffer) {
        buffer->Bind();
    }
}

void RenderContextGLES::BindUniformBuffer(IBufferImpl* buffer, uint32_t slot) {
    LR_LOG_TRACE_F("OpenGL ES BindUniformBuffer: %p, slot=%u", buffer, slot);
    if (buffer) {
        static_cast<gles::UniformBufferGLES*>(buffer)->BindToSlot(slot);
    }
}

void RenderContextGLES::BindTexture(ITextureImpl* texture, uint32_t slot) {
    LR_LOG_TRACE_F("OpenGL ES BindTexture: %p, slot=%u", texture, slot);
    if (texture) {
        texture->Bind(slot);
    }
}

void RenderContextGLES::BeginRenderPass(IFrameBufferImpl* frameBuffer) {
    LR_LOG_TRACE_F("OpenGL ES BeginRenderPass: FBO=%p", frameBuffer);
    
    m_currentFrameBuffer = frameBuffer;
    
    // 重要：确保深度写入启用，以便Clear能正常工作
    glDepthMask(GL_TRUE);
    LR_LOG_TRACE("  Depth Write: Forced to ENABLED for Clear operation");
    
    if (frameBuffer) {
        // 绑定离屏FBO，并设置视口
        frameBuffer->Bind();
        LR_LOG_TRACE_F("  Viewport: %ux%u (offscreen FBO)", 
                       frameBuffer->GetWidth(), frameBuffer->GetHeight());
    } else {
        // 绑定默认framebuffer（屏幕）
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // 重要：恢复默认视口
        glViewport(0, 0, m_width, m_height);
        LR_LOG_TRACE_F("  Viewport: %ux%u (default FBO/screen)", m_width, m_height);
    }
}

void RenderContextGLES::EndRenderPass() {
    LR_LOG_TRACE("OpenGL ES EndRenderPass");
    
    // 移动平台优化：可以使用glInvalidateFramebuffer丢弃不需要的附件
    // 这对于tile-based GPU很有用
    if (m_capabilities.hasDiscardFramebuffer && m_currentFrameBuffer != nullptr) {
        // 可以在这里添加帧缓冲失效优化
    }
    
    // 确保命令提交
    glFlush();
    LR_LOG_TRACE("  glFlush() called to ensure command submission");
    
    m_currentFrameBuffer = nullptr;
}

void RenderContextGLES::DrawArrays(PrimitiveType primitiveType, 
                                   uint32_t vertexStart, uint32_t vertexCount) {
    LR_LOG_TRACE_F("OpenGL ES DrawArrays: type=%d, start=%u, count=%u", 
                   (int)primitiveType, vertexStart, vertexCount);
    GLenum mode = gles::ToGLESPrimitiveType(primitiveType);
    glDrawArrays(mode, vertexStart, vertexCount);
}

void RenderContextGLES::DrawElements(PrimitiveType primitiveType, uint32_t indexCount,
                                     IndexType indexType, size_t indexOffset) {
    LR_LOG_TRACE_F("OpenGL ES DrawElements: type=%d, count=%u, indexType=%d, offset=%zu", 
                   (int)primitiveType, indexCount, (int)indexType, indexOffset);
    GLenum mode = gles::ToGLESPrimitiveType(primitiveType);
    GLenum type = gles::ToGLESIndexType(indexType);
    glDrawElements(mode, indexCount, type, reinterpret_cast<const void*>(indexOffset));
}

void RenderContextGLES::DrawArraysInstanced(PrimitiveType primitiveType, 
                                            uint32_t vertexStart,
                                            uint32_t vertexCount, 
                                            uint32_t instanceCount) {
    LR_LOG_TRACE_F("OpenGL ES DrawArraysInstanced: type=%d, start=%u, count=%u, instances=%u",
                   (int)primitiveType, vertexStart, vertexCount, instanceCount);
    GLenum mode = gles::ToGLESPrimitiveType(primitiveType);
    glDrawArraysInstanced(mode, vertexStart, vertexCount, instanceCount);
}

void RenderContextGLES::DrawElementsInstanced(PrimitiveType primitiveType, 
                                              uint32_t indexCount,
                                              IndexType indexType, 
                                              size_t indexOffset,
                                              uint32_t instanceCount) {
    LR_LOG_TRACE_F("OpenGL ES DrawElementsInstanced: type=%d, count=%u, indexType=%d, "
                   "offset=%zu, instances=%u",
                   (int)primitiveType, indexCount, (int)indexType, indexOffset, instanceCount);
    GLenum mode = gles::ToGLESPrimitiveType(primitiveType);
    GLenum type = gles::ToGLESIndexType(indexType);
    glDrawElementsInstanced(mode, indexCount, type, 
                           reinterpret_cast<const void*>(indexOffset), instanceCount);
}

void RenderContextGLES::WaitIdle() {
    glFinish();
}

void RenderContextGLES::Flush() {
    glFlush();
}

} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGLES
