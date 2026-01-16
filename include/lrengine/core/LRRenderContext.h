/**
 * @file LRRenderContext.h
 * @brief LREngine渲染上下文
 */

#pragma once

#include "LRDefines.h"
#include "LRTypes.h"

#include <memory>

namespace lrengine {
namespace render {

// 前向声明
class IRenderContextImpl;
class LRBuffer;
class LRVertexBuffer;
class LRIndexBuffer;
class LRUniformBuffer;
class LRShader;
class LRShaderProgram;
class LRTexture;
class LRFrameBuffer;
class LRPipelineState;
class LRFence;

/**
 * @brief 渲染上下文类
 * 
 * 渲染系统的核心入口点，提供：
 * - 资源创建工厂方法
 * - 渲染状态管理
 * - 绘制命令提交
 */
class LR_API LRRenderContext {
public:
    LR_NONCOPYABLE(LRRenderContext);
    
    /**
     * @brief 创建渲染上下文
     * @param desc 上下文描述符
     * @return 上下文实例，失败返回nullptr
     */
    static LRRenderContext* Create(const RenderContextDescriptor& desc);
    
    /**
     * @brief 销毁渲染上下文
     */
    static void Destroy(LRRenderContext* context);
    
    ~LRRenderContext();
    
    // =========================================================================
    // 资源创建
    // =========================================================================
    
    /**
     * @brief 创建通用缓冲区
     */
    LRBuffer* CreateBuffer(const BufferDescriptor& desc);
    
    /**
     * @brief 创建顶点缓冲区
     */
    LRVertexBuffer* CreateVertexBuffer(const BufferDescriptor& desc);
    
    /**
     * @brief 创建索引缓冲区
     */
    LRIndexBuffer* CreateIndexBuffer(const BufferDescriptor& desc);
    
    /**
     * @brief 创建统一缓冲区
     */
    LRUniformBuffer* CreateUniformBuffer(const BufferDescriptor& desc);
    
    /**
     * @brief 创建着色器
     */
    LRShader* CreateShader(const ShaderDescriptor& desc);
    
    
    /**
     * @brief 创建着色器程序
     * @param vertexShader 顶点着色器
     * @param fragmentShader 片段着色器
     * @param geometryShader 几何着色器（可选）
     */
    LRShaderProgram* CreateShaderProgram(LRShader* vertexShader, 
                                        LRShader* fragmentShader,
                                        LRShader* geometryShader = nullptr);
    
    /**
     * @brief 创建纹理
     */
    LRTexture* CreateTexture(const TextureDescriptor& desc);
    
    /**
     * @brief 创建帧缓冲
     */
    LRFrameBuffer* CreateFrameBuffer(const FrameBufferDescriptor& desc);
    
    /**
     * @brief 创建管线状态
     */
    LRPipelineState* CreatePipelineState(const PipelineStateDescriptor& desc);
    
    /**
     * @brief 创建同步栅栏
     */
    LRFence* CreateFence();
    
    // =========================================================================
    // 帧控制
    // =========================================================================
    
    /**
     * @brief 开始新帧
     */
    void BeginFrame();
    
    /**
     * @brief 结束当前帧
     */
    void EndFrame();
    
    /**
     * @brief 呈现帧（交换缓冲区）
     */
    void Present();
    
    // =========================================================================
    // 渲染目标
    // =========================================================================
    
    /**
     * @brief 开始渲染通道
     * @param frameBuffer 渲染目标，nullptr表示默认帧缓冲
     */
    void BeginRenderPass(LRFrameBuffer* frameBuffer = nullptr);
    
    /**
     * @brief 结束渲染通道
     */
    void EndRenderPass();
    
    // =========================================================================
    // 渲染状态
    // =========================================================================
    
    /**
     * @brief 设置视口
     */
    void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height);
    
    /**
     * @brief 设置视口（全屏）
     */
    void SetViewport(int32_t width, int32_t height) {
        SetViewport(0, 0, width, height);
    }
    
    /**
     * @brief 设置裁剪矩形
     */
    void SetScissor(int32_t x, int32_t y, int32_t width, int32_t height);
    
    /**
     * @brief 设置管线状态
     */
    void SetPipelineState(LRPipelineState* pipelineState);
    
    /**
     * @brief 设置顶点缓冲区
     * @param buffer 顶点缓冲区
     * @param slot 绑定槽位
     */
    void SetVertexBuffer(LRVertexBuffer* buffer, uint32_t slot = 0);
    
    /**
     * @brief 设置索引缓冲区
     */
    void SetIndexBuffer(LRIndexBuffer* buffer);
    
    /**
     * @brief 设置统一缓冲区
     * @param buffer 统一缓冲区
     * @param slot 绑定槽位
     */
    void SetUniformBuffer(LRUniformBuffer* buffer, uint32_t slot);
    
    /**
     * @brief 设置纹理
     * @param texture 纹理对象
     * @param slot 纹理单元
     */
    void SetTexture(LRTexture* texture, uint32_t slot);
    
    // =========================================================================
    // 清除
    // =========================================================================
    
    /**
     * @brief 清除颜色缓冲区
     * @param r, g, b, a 清除颜色
     */
    void ClearColor(float r, float g, float b, float a);
    
    /**
     * @brief 清除深度缓冲区
     * @param depth 清除深度值
     */
    void ClearDepth(float depth = 1.0f);
    
    /**
     * @brief 清除模板缓冲区
     * @param stencil 清除模板值
     */
    void ClearStencil(uint8_t stencil = 0);
    
    /**
     * @brief 清除所有缓冲区
     */
    void Clear(uint8_t flags, float r, float g, float b, float a, float depth = 1.0f, uint8_t stencil = 0);
    
    // =========================================================================
    // 绘制
    // =========================================================================
    
    /**
     * @brief 绘制图元
     * @param vertexStart 起始顶点
     * @param vertexCount 顶点数量
     */
    void Draw(uint32_t vertexStart, uint32_t vertexCount);
    
    /**
     * @brief 索引绘制
     * @param indexStart 起始索引
     * @param indexCount 索引数量
     */
    void DrawIndexed(uint32_t indexStart, uint32_t indexCount);
    
    /**
     * @brief 实例化绘制
     * @param vertexStart 起始顶点
     * @param vertexCount 顶点数量
     * @param instanceCount 实例数量
     */
    void DrawInstanced(uint32_t vertexStart, uint32_t vertexCount, uint32_t instanceCount);
    
    /**
     * @brief 实例化索引绘制
     * @param indexStart 起始索引
     * @param indexCount 索引数量
     * @param instanceCount 实例数量
     */
    void DrawIndexedInstanced(uint32_t indexStart, uint32_t indexCount, uint32_t instanceCount);
    
    // =========================================================================
    // 同步
    // =========================================================================
    
    /**
     * @brief 等待GPU空闲
     */
    void WaitIdle();
    
    /**
     * @brief 刷新命令
     */
    void Flush();
    
    // =========================================================================
    // 查询
    // =========================================================================
    
    /**
     * @brief 获取后端类型
     */
    Backend GetBackend() const { return mBackend; }
    
    /**
     * @brief 获取窗口宽度
     */
    uint32_t GetWidth() const { return mWidth; }
    
    /**
     * @brief 获取窗口高度
     */
    uint32_t GetHeight() const { return mHeight; }
    
    /**
     * @brief 激活当前上下文
     */
    void MakeCurrent();
    
private:
    LRRenderContext();
    bool Initialize(const RenderContextDescriptor& desc);
    void Shutdown();
    
private:
    IRenderContextImpl* mImpl = nullptr;
    Backend mBackend = Backend::Unknown;
    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    
    // 当前状态
    LRPipelineState* mCurrentPipelineState = nullptr;
    LRFrameBuffer* mCurrentFrameBuffer = nullptr;
    PrimitiveType mCurrentPrimitiveType = PrimitiveType::Triangles;
    IndexType mCurrentIndexType = IndexType::UInt32;
};

} // namespace render
} // namespace lrengine
