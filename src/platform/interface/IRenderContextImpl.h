/**
 * @file IRenderContextImpl.h
 * @brief 渲染上下文平台实现接口
 */

#pragma once

#include "lrengine/core/LRTypes.h"

namespace lrengine {
namespace render {

// 前向声明
class IBufferImpl;
class IShaderImpl;
class IShaderProgramImpl;
class ITextureImpl;
class IFrameBufferImpl;
class IPipelineStateImpl;
class IFenceImpl;

/**
 * @brief 渲染上下文实现接口
 * 
 * 定义平台后端必须实现的核心渲染功能
 */
class IRenderContextImpl {
public:
    virtual ~IRenderContextImpl() = default;

    // =========================================================================
    // 初始化和状态
    // =========================================================================

    /**
     * @brief 初始化渲染上下文
     * @param desc 上下文描述符
     * @return 成功返回true
     */
    virtual bool Initialize(const RenderContextDescriptor& desc) = 0;

    /**
     * @brief 关闭渲染上下文
     */
    virtual void Shutdown() = 0;

    /**
     * @brief 激活当前上下文
     */
    virtual void MakeCurrent() = 0;

    /**
     * @brief 交换缓冲区
     */
    virtual void SwapBuffers() = 0;

    /**
     * @brief 开始新帧
     */
    virtual void BeginFrame() {}

    /**
     * @brief 结束当前帧
     */
    virtual void EndFrame() {}

    /**
     * @brief 获取后端类型
     */
    virtual Backend GetBackend() const = 0;

    // =========================================================================
    // 资源创建
    // =========================================================================

    /**
     * @brief 创建缓冲区实现
     * @param type 缓冲区类型
     */
    virtual IBufferImpl* CreateBufferImpl(BufferType type = BufferType::Vertex) = 0;

    /**
     * @brief 创建着色器实现
     */
    virtual IShaderImpl* CreateShaderImpl() = 0;

    /**
     * @brief 创建着色器程序实现
     */
    virtual IShaderProgramImpl* CreateShaderProgramImpl() = 0;

    /**
     * @brief 创建纹理实现
     */
    virtual ITextureImpl* CreateTextureImpl() = 0;

    /**
     * @brief 创建帧缓冲实现
     */
    virtual IFrameBufferImpl* CreateFrameBufferImpl() = 0;

    /**
     * @brief 创建管线状态实现
     */
    virtual IPipelineStateImpl* CreatePipelineStateImpl() = 0;

    /**
     * @brief 创建栅栏实现
     */
    virtual IFenceImpl* CreateFenceImpl() = 0;

    // =========================================================================
    // 渲染状态
    // =========================================================================

    /**
     * @brief 设置视口
     */
    virtual void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height) = 0;

    /**
     * @brief 设置裁剪矩形
     */
    virtual void SetScissor(int32_t x, int32_t y, int32_t width, int32_t height) = 0;

    /**
     * @brief 清除缓冲区
     * @param flags 清除标志
     * @param color 清除颜色
     * @param depth 清除深度
     * @param stencil 清除模板
     */
    virtual void Clear(uint8_t flags, const float* color, float depth, uint8_t stencil) = 0;

    /**
     * @brief 绑定管线状态
     * @param pipelineState 管线状态实现
     */
    virtual void BindPipelineState(IPipelineStateImpl* pipelineState) {}

    /**
     * @brief 绑定顶点缓冲区
     * @param buffer 缓冲区实现
     * @param slot 绑定槽位
     */
    virtual void BindVertexBuffer(IBufferImpl* buffer, uint32_t slot) {}

    /**
     * @brief 绑定索引缓冲区
     * @param buffer 缓冲区实现
     */
    virtual void BindIndexBuffer(IBufferImpl* buffer) {}

    /**
     * @brief 绑定Uniform缓冲区
     * @param buffer 缓冲区实现
     * @param slot 绑定槽位
     */
    virtual void BindUniformBuffer(IBufferImpl* buffer, uint32_t slot) {}

    /**
     * @brief 绑定纹理
     * @param texture 纹理实现
     * @param slot 绑定槽位
     */
    virtual void BindTexture(ITextureImpl* texture, uint32_t slot) {}

    /**
     * @brief 开始渲染通道
     * @param frameBuffer 帧缓冲实现，nullptr表示默认framebuffer
     */
    virtual void BeginRenderPass(IFrameBufferImpl* frameBuffer) {}

    /**
     * @brief 结束渲染通道
     */
    virtual void EndRenderPass() {}

    // =========================================================================
    // 绘制命令
    // =========================================================================

    /**
     * @brief 绘制图元
     * @param primitiveType 图元类型
     * @param vertexStart 起始顶点
     * @param vertexCount 顶点数量
     */
    virtual void DrawArrays(PrimitiveType primitiveType,
                            uint32_t vertexStart,
                            uint32_t vertexCount) = 0;

    /**
     * @brief 索引绘制
     * @param primitiveType 图元类型
     * @param indexCount 索引数量
     * @param indexType 索引类型
     * @param indexOffset 索引偏移
     */
    virtual void DrawElements(PrimitiveType primitiveType,
                              uint32_t indexCount,
                              IndexType indexType,
                              size_t indexOffset) = 0;

    /**
     * @brief 实例化绘制
     */
    virtual void DrawArraysInstanced(PrimitiveType primitiveType,
                                     uint32_t vertexStart,
                                     uint32_t vertexCount,
                                     uint32_t instanceCount) = 0;

    /**
     * @brief 实例化索引绘制
     */
    virtual void DrawElementsInstanced(PrimitiveType primitiveType,
                                       uint32_t indexCount,
                                       IndexType indexType,
                                       size_t indexOffset,
                                       uint32_t instanceCount) = 0;

    // =========================================================================
    // 同步
    // =========================================================================

    /**
     * @brief 等待GPU空闲
     */
    virtual void WaitIdle() = 0;

    /**
     * @brief 刷新命令
     */
    virtual void Flush() = 0;
};

} // namespace render
} // namespace lrengine
