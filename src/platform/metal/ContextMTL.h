/**
 * @file ContextMTL.h
 * @brief Metal渲染上下文
 */

#pragma once

#include "platform/interface/IRenderContextImpl.h"

#ifdef LRENGINE_ENABLE_METAL

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <memory>
#include <vector>

namespace lrengine {
namespace render {
namespace mtl {

class BufferMTL;
class VertexBufferMTL;
class IndexBufferMTL;
class UniformBufferMTL;
class TextureMTL;
class PipelineStateMTL;
class FrameBufferMTL;
class RenderPassMTL;
class CommandBufferPoolMTL;
class RenderStateStackMTL;

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
    void BeginRenderPass(IFrameBufferImpl* frameBuffer) override;
    void EndRenderPass() override;

    void DrawArrays(PrimitiveType primitiveType, uint32_t vertexStart, uint32_t vertexCount) override;
    void DrawElements(PrimitiveType primitiveType,
                      uint32_t indexCount,
                      IndexType indexType,
                      size_t indexOffset) override;
    void DrawArraysInstanced(PrimitiveType primitiveType,
                             uint32_t vertexStart,
                             uint32_t vertexCount,
                             uint32_t instanceCount) override;
    void DrawElementsInstanced(PrimitiveType primitiveType,
                               uint32_t indexCount,
                               IndexType indexType,
                               size_t indexOffset,
                               uint32_t instanceCount) override;

    void WaitIdle() override;
    void Flush() override;

    void BeginFrame() override;
    void EndFrame() override;

    // =====================================================================
    // 新渲染通道API（推荐使用）
    // =====================================================================

    /**
     * @brief 开始渲染通道（新版本）
     * @param renderPass 渲染通道配置，nullptr表示默认帧缓冲
     * @note 此方法会立即创建encoder，之后的绑定操作生效
     */
    void BeginRenderPassEx(RenderPassMTL* renderPass);

    /**
     * @brief 结束渲染通道（新版本）
     */
    void EndRenderPassEx();

    /**
     * @brief 开始多渲染目标（MRT）渲染通道
     * @param colorTargets 多个颜色目标（支持最多8个）
     * @param depthTarget 深度目标（可选）
     */
    void BeginMRTRenderPass(const std::vector<FrameBufferMTL*>& colorTargets,
                            FrameBufferMTL* depthTarget = nullptr);

    /**
     * @brief 便捷方法：开始单个离屏渲染目标
     */
    void BeginOffscreenRenderPass(FrameBufferMTL* target, FrameBufferMTL* depthTarget = nullptr);

    // =====================================================================
    // Metal特有方法
    // =====================================================================

    id<MTLDevice> GetDevice() const { return m_device; }
    id<MTLCommandQueue> GetCommandQueue() const { return m_commandQueue; }
    id<MTLCommandBuffer> GetCurrentCommandBuffer();
    id<MTLRenderCommandEncoder> GetCurrentRenderEncoder();

    // CommandBufferPool访问（用于多线程渲染）
    CommandBufferPoolMTL* GetCommandBufferPool() const { return m_commandBufferPool.get(); }

    // 状态绑定
    void SetVertexBuffer(VertexBufferMTL* buffer, uint32_t index);
    void SetIndexBuffer(IndexBufferMTL* buffer);
    void SetPipelineState(PipelineStateMTL* pipelineState);
    void SetUniformBuffer(UniformBufferMTL* buffer, uint32_t index);
    void SetTexture(TextureMTL* texture, uint32_t slot);

private:
    // =====================================================================
    // 内部辅助方法
    // =====================================================================

    void CreateDepthStencilTexture();

    /**
     * @brief 创建RenderEncoder（新版本）
     * @param renderPass 渲染通道
     * @return 创建的encoder
     */
    id<MTLRenderCommandEncoder> CreateRenderEncoder(RenderPassMTL* renderPass);

    /**
     * @brief 结束当前encoder
     */
    void EndCurrentEncoder();

    /**
     * @brief 应用视口和裁剪到当前encoder
     */
    void ApplyViewportAndScissor();

    /**
     * @brief 恢复管线状态到encoder
     */
    void RestorePipelineState();

    // =====================================================================
    // Metal核心对象
    // =====================================================================

    id<MTLDevice> m_device;
    id<MTLCommandQueue> m_commandQueue;

    CAMetalLayer* m_metalLayer;
    id<CAMetalDrawable> m_currentDrawable;
    id<MTLTexture> m_depthStencilTexture;

    // =====================================================================
    // 新架构：CommandBuffer池和状态栈
    // =====================================================================

    // CommandBuffer池（支持多线程）
    std::unique_ptr<CommandBufferPoolMTL> m_commandBufferPool;

    // 渲染状态栈（支持嵌套RenderPass）
    std::unique_ptr<RenderStateStackMTL> m_stateStack;

    // 当前RenderPass配置
    std::unique_ptr<RenderPassMTL> m_currentRenderPass;

    // 默认RenderPass（用于屏幕渲染）
    std::unique_ptr<RenderPassMTL> m_defaultRenderPass;

    // =====================================================================
    // 基础配置
    // =====================================================================

    void* m_windowHandle;
    uint32_t m_width;
    uint32_t m_height;
    bool m_debug;
    bool m_vsync;

    // =====================================================================
    // 帧状态
    // =====================================================================

    bool m_frameStarted;
    id<MTLCommandBuffer> m_frameCommandBuffer; // 整个帧共享的CommandBuffer

    MTLViewport m_viewport;
    MTLScissorRect m_scissorRect;
    bool m_viewportDirty;
    bool m_scissorDirty;

    // =====================================================================
    // 清除值
    // =====================================================================

    float m_clearColor[4];
    float m_clearDepth;
    uint8_t m_clearStencil;

    // =====================================================================
    // 当前绑定状态
    // =====================================================================

    VertexBufferMTL* m_currentVertexBuffer;
    IndexBufferMTL* m_currentIndexBuffer;
    PipelineStateMTL* m_currentPipelineState;
};

} // namespace mtl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
