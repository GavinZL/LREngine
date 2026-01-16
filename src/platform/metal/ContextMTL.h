/**
 * @file ContextMTL.h
 * @brief Metal渲染上下文
 */

#pragma once

#include "platform/interface/IRenderContextImpl.h"

#ifdef LRENGINE_ENABLE_METAL

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

namespace lrengine {
namespace render {
namespace mtl {

class BufferMTL;
class VertexBufferMTL;
class IndexBufferMTL;
class UniformBufferMTL;
class TextureMTL;
class PipelineStateMTL;

/**
 * @brief Metal渲染上下文实现
 */
class RenderContextMTL : public IRenderContextImpl {
public:
    RenderContextMTL();
    ~RenderContextMTL() override;
    
    // IRenderContextImpl接口实现
    bool Initialize(const RenderContextDescriptor& desc) override;
    void Shutdown() override;
    void MakeCurrent() override;
    void SwapBuffers() override;
    Backend GetBackend() const override { return Backend::Metal; }
    
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
    
    void BindPipelineState(IPipelineStateImpl* pipelineState) override;
    void BindVertexBuffer(IBufferImpl* buffer, uint32_t slot) override;
    void BindIndexBuffer(IBufferImpl* buffer) override;
    void BindUniformBuffer(IBufferImpl* buffer, uint32_t slot) override;
    void BindTexture(ITextureImpl* texture, uint32_t slot) override;
    
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
    
    void BeginFrame() override;
    void EndFrame() override;

    // Metal特有方法
    id<MTLDevice> GetDevice() const { return m_device; }
    id<MTLCommandQueue> GetCommandQueue() const { return m_commandQueue; }
    id<MTLCommandBuffer> GetCurrentCommandBuffer();
    id<MTLRenderCommandEncoder> GetCurrentRenderEncoder();
    
    // 状态绑定
    void SetVertexBuffer(VertexBufferMTL* buffer, uint32_t index);
    void SetIndexBuffer(IndexBufferMTL* buffer);
    void SetPipelineState(PipelineStateMTL* pipelineState);
    void SetUniformBuffer(UniformBufferMTL* buffer, uint32_t index);
    void SetTexture(TextureMTL* texture, uint32_t slot);

private:
    void CreateDepthStencilTexture();
    void EnsureRenderEncoder();
    void EndCurrentRenderEncoder();

    id<MTLDevice> m_device;
    id<MTLCommandQueue> m_commandQueue;
    id<MTLCommandBuffer> m_currentCommandBuffer;
    id<MTLRenderCommandEncoder> m_currentRenderEncoder;
    
    CAMetalLayer* m_metalLayer;
    id<CAMetalDrawable> m_currentDrawable;
    id<MTLTexture> m_depthStencilTexture;
    
    void* m_windowHandle;
    uint32_t m_width;
    uint32_t m_height;
    bool m_debug;
    bool m_vsync;
    
    // 视口和裁剪
    MTLViewport m_viewport;
    MTLScissorRect m_scissorRect;
    bool m_viewportDirty;
    bool m_scissorDirty;
    
    // 清除值
    float m_clearColor[4];
    float m_clearDepth;
    uint8_t m_clearStencil;
    
    // 当前绑定状态
    VertexBufferMTL* m_currentVertexBuffer;
    IndexBufferMTL* m_currentIndexBuffer;
    PipelineStateMTL* m_currentPipelineState;
    
    // 帧状态
    bool m_frameStarted;
};

} // namespace mtl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
