/**
 * @file ContextGL.h
 * @brief OpenGL渲染上下文
 */

#pragma once

#include "platform/interface/IRenderContextImpl.h"

#ifdef LRENGINE_ENABLE_OPENGL

#if defined(LR_PLATFORM_APPLE)
    #include <OpenGL/gl3.h>
#elif defined(LR_PLATFORM_WINDOWS)
    #include <glad/glad.h>
#else
    #include <GL/gl.h>
#endif

namespace lrengine {
namespace render {

/**
 * @brief OpenGL渲染上下文实现
 */
class RenderContextGL : public IRenderContextImpl {
public:
    RenderContextGL();
    ~RenderContextGL() override;
    
    // IRenderContextImpl接口实现
    bool Initialize(const RenderContextDescriptor& desc) override;
    void Shutdown() override;
    void MakeCurrent() override;
    void SwapBuffers() override;
    Backend GetBackend() const override { return Backend::OpenGL; }
    
    IBufferImpl* CreateBufferImpl(BufferType type = BufferType::Vertex) override;
    IShaderImpl* CreateShaderImpl() override;
    IShaderProgramImpl* CreateShaderProgramImpl() override;
    ITextureImpl* CreateTextureImpl() override;
    IFrameBufferImpl* CreateFrameBufferImpl() override;
    IPipelineStateImpl* CreatePipelineStateImpl() override;
    IFenceImpl* CreateFenceImpl() override;
    
    void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void SetScissor(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void Clear(uint8_t flags, const float* color, float depth, uint8_t stencil) override;
    
    void DrawArrays(PrimitiveType primitiveType, uint32_t vertexStart, uint32_t vertexCount) override;
    void DrawElements(PrimitiveType primitiveType, uint32_t indexCount, 
                     IndexType indexType, size_t indexOffset) override;
    void DrawArraysInstanced(PrimitiveType primitiveType, uint32_t vertexStart,
                            uint32_t vertexCount, uint32_t instanceCount) override;
    void DrawElementsInstanced(PrimitiveType primitiveType, uint32_t indexCount,
                              IndexType indexType, size_t indexOffset,
                              uint32_t instanceCount) override;
    
    void WaitIdle() override;
    void Flush() override;
    
private:
    void* mWindowHandle = nullptr;
    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    bool mDebug = false;
    
    // 默认VAO（OpenGL Core Profile需要）
    GLuint mDefaultVAO = 0;
};

} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGL
