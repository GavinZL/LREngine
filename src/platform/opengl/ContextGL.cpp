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

void RenderContextGL::DrawArrays(PrimitiveType primitiveType, uint32_t vertexStart, uint32_t vertexCount) {
    GLenum mode = gl::ToGLPrimitiveType(primitiveType);
    glDrawArrays(mode, vertexStart, vertexCount);
}

void RenderContextGL::DrawElements(PrimitiveType primitiveType, uint32_t indexCount,
                                  IndexType indexType, size_t indexOffset) {
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
