/**
 * @file ContextGLES.h
 * @brief OpenGL ES渲染上下文
 */

#pragma once

#include "platform/interface/IRenderContextImpl.h"
#include "GLESCapabilities.h"

#ifdef LRENGINE_ENABLE_OPENGLES

namespace lrengine {
namespace render {

// 前向声明
namespace gles {
class BufferGLES;
class VertexBufferGLES;
class IndexBufferGLES;
class UniformBufferGLES;
class ShaderGLES;
class ShaderProgramGLES;
class TextureGLES;
class FrameBufferGLES;
class PipelineStateGLES;
class FenceGLES;
}

/**
 * @brief OpenGL ES渲染上下文实现
 */
class RenderContextGLES : public IRenderContextImpl {
public:
    RenderContextGLES();
    ~RenderContextGLES() override;
    
    // =========================================================================
    // IRenderContextImpl接口实现
    // =========================================================================
    
    bool Initialize(const RenderContextDescriptor& desc) override;
    void Shutdown() override;
    void MakeCurrent() override;
    void SwapBuffers() override;
    void BeginFrame() override;
    void EndFrame() override;
    Backend GetBackend() const override { return Backend::OpenGLES; }
    
    // 资源创建
    IBufferImpl* CreateBufferImpl(BufferType type = BufferType::Vertex) override;
    IShaderImpl* CreateShaderImpl() override;
    IShaderProgramImpl* CreateShaderProgramImpl() override;
    ITextureImpl* CreateTextureImpl() override;
    IFrameBufferImpl* CreateFrameBufferImpl() override;
    IPipelineStateImpl* CreatePipelineStateImpl() override;
    IFenceImpl* CreateFenceImpl() override;
    
    // 渲染状态
    void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void SetScissor(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void Clear(uint8_t flags, const float* color, float depth, uint8_t stencil) override;
    
    void BindPipelineState(IPipelineStateImpl* pipelineState) override;
    void BindVertexBuffer(IBufferImpl* buffer, uint32_t slot) override;
    void BindIndexBuffer(IBufferImpl* buffer) override;
    void BindUniformBuffer(IBufferImpl* buffer, uint32_t slot) override;
    void BindTexture(ITextureImpl* texture, uint32_t slot) override;
    
    // 渲染通道
    void BeginRenderPass(IFrameBufferImpl* frameBuffer) override;
    void EndRenderPass() override;
    
    // 绘制
    void DrawArrays(PrimitiveType primitiveType, uint32_t vertexStart, uint32_t vertexCount) override;
    void DrawElements(PrimitiveType primitiveType, uint32_t indexCount, 
                     IndexType indexType, size_t indexOffset) override;
    void DrawArraysInstanced(PrimitiveType primitiveType, uint32_t vertexStart,
                            uint32_t vertexCount, uint32_t instanceCount) override;
    void DrawElementsInstanced(PrimitiveType primitiveType, uint32_t indexCount,
                              IndexType indexType, size_t indexOffset,
                              uint32_t instanceCount) override;
    
    // 同步
    void WaitIdle() override;
    void Flush() override;
    
    // =========================================================================
    // OpenGL ES特有方法
    // =========================================================================
    
    /**
     * @brief 获取能力检测结果
     */
    const gles::GLESCapabilities& GetCapabilities() const { return m_capabilities; }
    
    /**
     * @brief 获取默认VAO
     */
    GLuint GetDefaultVAO() const { return m_defaultVAO; }
    
private:
    void* m_windowHandle = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    bool m_debug = false;
    
    // 默认VAO
    GLuint m_defaultVAO = 0;
    
    // 能力检测
    gles::GLESCapabilities m_capabilities;
    
    // 当前绑定的帧缓冲
    IFrameBufferImpl* m_currentFrameBuffer = nullptr;
    
    // BeginRenderPass之前绑定的FBO ID（用于EndRenderPass恢复，iOS需要）
    GLint m_previousFrameBuffer = 0;
};

} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGLES
