/**
 * @file ContextMTL.mm
 * @brief Metal渲染上下文实现（重构版）
 * 
 * 重构特性：
 * - 支持多渲染目标(MRT)
 * - 支持多线程命令记录
 * - 使用RenderPass抽象管理渲染流程
 * - 使用CommandBufferPool提升性能
 * - 使用RenderStateStack支持嵌套渲染
 */

#include "ContextMTL.h"

#ifdef LRENGINE_ENABLE_METAL

#include "BufferMTL.h"
#include "ShaderMTL.h"
#include "TextureMTL.h"
#include "FrameBufferMTL.h"
#include "PipelineStateMTL.h"
#include "FenceMTL.h"
#include "TypeConverterMTL.h"
#include "RenderPassMTL.h"
#include "CommandBufferPoolMTL.h"
#include "RenderStateStackMTL.h"
#include "lrengine/core/LRError.h"
#include "lrengine/utils/LRLog.h"

// 平台特定的头文件
#if TARGET_OS_IPHONE || TARGET_OS_IOS
    #import <UIKit/UIKit.h>
#else
    #import <Cocoa/Cocoa.h>
#endif

namespace lrengine {
namespace render {
namespace mtl {

// =============================================================================
// 构造和析构
// =============================================================================

RenderContextMTL::RenderContextMTL()
    : m_device(nil)
    , m_commandQueue(nil)
    , m_metalLayer(nil)
    , m_currentDrawable(nil)
    , m_depthStencilTexture(nil)
    , m_windowHandle(nullptr)
    , m_width(0)
    , m_height(0)
    , m_debug(false)
    , m_vsync(true)
    , m_viewportDirty(true)
    , m_scissorDirty(true)
    , m_clearDepth(1.0f)
    , m_clearStencil(0)
    , m_currentVertexBuffer(nullptr)
    , m_currentIndexBuffer(nullptr)
    , m_currentPipelineState(nullptr)
    , m_frameStarted(false)
{
    m_clearColor[0] = 0.0f;
    m_clearColor[1] = 0.0f;
    m_clearColor[2] = 0.0f;
    m_clearColor[3] = 1.0f;
    
    m_viewport = {0, 0, 0, 0, 0, 1};
    m_scissorRect = {0, 0, 0, 0};
}

RenderContextMTL::~RenderContextMTL() {
    Shutdown();
}

// =============================================================================
// 初始化和清理
// =============================================================================

bool RenderContextMTL::Initialize(const RenderContextDescriptor& desc) {
    m_windowHandle = desc.windowHandle;
    m_width = desc.width;
    m_height = desc.height;
    m_debug = desc.debug;
    m_vsync = desc.vsync;

    // 获取默认Metal设备
    m_device = MTLCreateSystemDefaultDevice();
    if (!m_device) {
        LR_SET_ERROR(ErrorCode::BackendNotAvailable, "Metal is not supported on this device");
        return false;
    }

    // 创建命令队列
    m_commandQueue = [m_device newCommandQueue];
    if (!m_commandQueue) {
        LR_SET_ERROR(ErrorCode::ResourceCreationFailed, "Failed to create Metal command queue");
        return false;
    }

    // 设置Metal层（如果提供了窗口句柄）
    if (m_windowHandle) {
        #if TARGET_OS_IPHONE || TARGET_OS_IOS
            // iOS平台: 检测传入的对象类型
            id windowHandle = (__bridge id)m_windowHandle;
            
            // 检查是否直接传入了CAMetalLayer
            if ([windowHandle isKindOfClass:[CAMetalLayer class]]) {
                // 直接使用传入的CAMetalLayer（适用于iOS应用直接集成）
                m_metalLayer = (CAMetalLayer*)windowHandle;
                m_metalLayer.device = m_device;
                m_metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
                m_metalLayer.framebufferOnly = YES;
                
                LR_LOG_INFO_F("Metal: Using provided CAMetalLayer directly");
            }
            else if ([windowHandle isKindOfClass:[UIWindow class]]) {
                // 传统方式: 使用UIWindow和UIView
                UIWindow* window = (UIWindow*)windowHandle;
                UIView* contentView = [window rootViewController].view;
                
                if (!contentView) {
                    contentView = window;
                }
                
                // 创建新的CAMetalLayer
                m_metalLayer = [CAMetalLayer layer];
                m_metalLayer.device = m_device;
                m_metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
                m_metalLayer.framebufferOnly = YES;
                
                // iOS使用UIScreen的scale
                CGFloat scale = [UIScreen mainScreen].nativeScale;
                m_metalLayer.contentsScale = scale;
                m_metalLayer.drawableSize = CGSizeMake(m_width * scale, m_height * scale);
                
                // 设置layer的frame
                m_metalLayer.frame = contentView.bounds;
                
                // 添加到视图层次
                [contentView.layer addSublayer:m_metalLayer];
                
                LR_LOG_INFO_F("Metal: Created CAMetalLayer from UIWindow");
            }
            else {
                LR_SET_ERROR(ErrorCode::InvalidArgument, "Invalid window handle type for iOS platform");
                return false;
            }
            
        #else
            // macOS平台: 使用NSWindow和NSView
            NSWindow* window = (__bridge NSWindow*)m_windowHandle;
            NSView* contentView = [window contentView];
            
            // 设置CAMetalLayer
            [contentView setWantsLayer:YES];
            m_metalLayer = [CAMetalLayer layer];
            m_metalLayer.device = m_device;
            m_metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
            m_metalLayer.framebufferOnly = YES;
            m_metalLayer.drawableSize = CGSizeMake(m_width, m_height);
            m_metalLayer.displaySyncEnabled = m_vsync;
            
            // macOS使用contentsScale适配Retina显示
            m_metalLayer.contentsScale = contentView.window.backingScaleFactor;
            
            [contentView setLayer:m_metalLayer];
        #endif
    }

    // 创建深度/模板纹理
    CreateDepthStencilTexture();
    
    // 初始化CommandBuffer池
    m_commandBufferPool = std::make_unique<CommandBufferPoolMTL>(m_commandQueue, 3);
    
    // 初始化状态栈
    m_stateStack = std::make_unique<RenderStateStackMTL>();

    // 设置默认视口
    m_viewport.originX = 0;
    m_viewport.originY = 0;
    m_viewport.width = m_width;
    m_viewport.height = m_height;
    m_viewport.znear = 0;
    m_viewport.zfar = 1;
    
    m_scissorRect.x = 0;
    m_scissorRect.y = 0;
    m_scissorRect.width = m_width;
    m_scissorRect.height = m_height;

    LR_LOG_INFO_F("ContextMTL: Initialized successfully (%ux%u)", m_width, m_height);
    return true;
}

void RenderContextMTL::Shutdown() {
    LR_LOG_INFO_F("ContextMTL: Shutting down...");
    
    // 清空状态栈
    if (m_stateStack) {
        m_stateStack->Clear();
    }
    
    // 等待所有CommandBuffer完成
    if (m_commandBufferPool) {
        m_commandBufferPool->WaitIdle();
    }
    
    // 清理资源
    m_currentRenderPass.reset();
    m_defaultRenderPass.reset();
    m_stateStack.reset();
    m_commandBufferPool.reset();
    
    m_depthStencilTexture = nil;
    m_currentDrawable = nil;
    m_metalLayer = nil;
    m_commandQueue = nil;
    m_device = nil;
    
    LR_LOG_INFO_F("ContextMTL: Shutdown completed");
}

void RenderContextMTL::MakeCurrent() {
    // Metal没有上下文切换的概念
}

void RenderContextMTL::SwapBuffers() {
    EndFrame();
    BeginFrame();
}

// =============================================================================
// 资源创建
// =============================================================================

IBufferImpl* RenderContextMTL::CreateBufferImpl(BufferType type) {
    switch (type) {
        case BufferType::Vertex:
            return new VertexBufferMTL(m_device);
        case BufferType::Index:
            return new IndexBufferMTL(m_device);
        case BufferType::Uniform:
            return new UniformBufferMTL(m_device);
        default:
            return new BufferMTL(m_device);
    }
}

IShaderImpl* RenderContextMTL::CreateShaderImpl() {
    return new ShaderMTL(m_device);
}

IShaderProgramImpl* RenderContextMTL::CreateShaderProgramImpl() {
    return new ShaderProgramMTL(m_device);
}

ITextureImpl* RenderContextMTL::CreateTextureImpl() {
    return new TextureMTL(m_device);
}

IFrameBufferImpl* RenderContextMTL::CreateFrameBufferImpl() {
    // 不再传递context指针，解除循环依赖
    return new FrameBufferMTL(m_device);
}

IPipelineStateImpl* RenderContextMTL::CreatePipelineStateImpl() {
    return new PipelineStateMTL(m_device);
}

IFenceImpl* RenderContextMTL::CreateFenceImpl() {
    return new FenceMTL(m_device);
}

// =============================================================================
// 视口和裁剪
// =============================================================================

void RenderContextMTL::SetViewport(int32_t x, int32_t y, int32_t width, int32_t height) {
    m_viewport.originX = x;
    m_viewport.originY = y;
    m_viewport.width = width;
    m_viewport.height = height;
    m_viewportDirty = true;
    
    // 应用到当前encoder
    if (!m_stateStack->IsEmpty()) {
        auto& state = m_stateStack->GetCurrentState();
        if (state.encoder) {
            [state.encoder setViewport:m_viewport];
        }
    }
}

void RenderContextMTL::SetScissor(int32_t x, int32_t y, int32_t width, int32_t height) {
    m_scissorRect.x = x;
    m_scissorRect.y = y;
    m_scissorRect.width = width;
    m_scissorRect.height = height;
    m_scissorDirty = true;
    
    // 应用到当前encoder
    if (!m_stateStack->IsEmpty()) {
        auto& state = m_stateStack->GetCurrentState();
        if (state.encoder) {
            [state.encoder setScissorRect:m_scissorRect];
        }
    }
}

// =============================================================================
// 清除操作
// =============================================================================

void RenderContextMTL::Clear(uint8_t flags, const float* color, float depth, uint8_t stencil) {
    LR_LOG_TRACE_F("ContextMTL: Clear called with flags: %u", flags);
    
    if (flags & ClearColor && color) {
        m_clearColor[0] = color[0];
        m_clearColor[1] = color[1];
        m_clearColor[2] = color[2];
        m_clearColor[3] = color[3];
    }
    
    if (flags & ClearDepth) {
        m_clearDepth = depth;
    }
    
    if (flags & ClearStencil) {
        m_clearStencil = stencil;
    }
    
    // 注意：在Metal中，清除值在创建RenderPassDescriptor时应用
    // 如果需要在渲染中途清除，需要结束当前pass并重新开始
}

// =============================================================================
// 状态绑定
// =============================================================================

void RenderContextMTL::BindPipelineState(IPipelineStateImpl* pipelineState) {
    m_currentPipelineState = static_cast<PipelineStateMTL*>(pipelineState);
    
    LR_LOG_TRACE_F("ContextMTL: Binding pipeline state");
    
    // 应用到当前encoder
    if (!m_stateStack->IsEmpty()) {
        auto& state = m_stateStack->GetCurrentState();
        if (state.encoder && m_currentPipelineState) {
            if (m_currentPipelineState->GetPipelineState()) {
                [state.encoder setRenderPipelineState:m_currentPipelineState->GetPipelineState()];
            }
            if (m_currentPipelineState->GetDepthStencilState()) {
                [state.encoder setDepthStencilState:m_currentPipelineState->GetDepthStencilState()];
            }
            [state.encoder setCullMode:m_currentPipelineState->GetCullMode()];
            [state.encoder setFrontFacingWinding:m_currentPipelineState->GetFrontFace()];
            [state.encoder setTriangleFillMode:m_currentPipelineState->GetFillMode()];
        }
    }
}

void RenderContextMTL::BindVertexBuffer(IBufferImpl* buffer, uint32_t slot) {
    m_currentVertexBuffer = static_cast<VertexBufferMTL*>(buffer);
    
    if (!m_stateStack->IsEmpty()) {
        auto& state = m_stateStack->GetCurrentState();
        if (state.encoder && m_currentVertexBuffer && m_currentVertexBuffer->GetBuffer()) {
            LR_LOG_TRACE_F("ContextMTL: Binding vertex buffer to slot %u", slot);
            [state.encoder setVertexBuffer:m_currentVertexBuffer->GetBuffer()
                                     offset:0
                                    atIndex:slot];
        }
    }
}

void RenderContextMTL::BindIndexBuffer(IBufferImpl* buffer) {
    m_currentIndexBuffer = static_cast<IndexBufferMTL*>(buffer);
}

void RenderContextMTL::BindUniformBuffer(IBufferImpl* buffer, uint32_t slot) {
    UniformBufferMTL* uniformBuffer = static_cast<UniformBufferMTL*>(buffer);
    SetUniformBuffer(uniformBuffer, slot);
}

void RenderContextMTL::BindTexture(ITextureImpl* texture, uint32_t slot) {
    TextureMTL* textureMTL = static_cast<TextureMTL*>(texture);
    SetTexture(textureMTL, slot);
}

// =============================================================================
// 绘制命令
// =============================================================================

void RenderContextMTL::DrawArrays(PrimitiveType primitiveType, uint32_t vertexStart, uint32_t vertexCount) {
    if (m_stateStack->IsEmpty()) {
        LR_LOG_ERROR_F("ContextMTL: DrawArrays called but no active render pass");
        return;
    }
    
    auto& state = m_stateStack->GetCurrentState();
    if (!state.encoder) {
        LR_LOG_ERROR_F("ContextMTL: DrawArrays called but no render encoder");
        return;
    }
    
    LR_LOG_TRACE_F("ContextMTL: DrawArrays - start: %u, count: %u", vertexStart, vertexCount);
    
    MTLPrimitiveType type = ToMTLPrimitiveType(primitiveType);
    [state.encoder drawPrimitives:type
                       vertexStart:vertexStart
                       vertexCount:vertexCount];
}

void RenderContextMTL::DrawElements(PrimitiveType primitiveType, uint32_t indexCount,
                                   IndexType indexType, size_t indexOffset) {
    if (m_stateStack->IsEmpty()) {
        LR_LOG_ERROR_F("ContextMTL: DrawElements called but no active render pass");
        return;
    }
    
    auto& state = m_stateStack->GetCurrentState();
    if (!state.encoder || !m_currentIndexBuffer) {
        LR_LOG_ERROR_F("ContextMTL: DrawElements called but no render encoder or index buffer");
        return;
    }
    
    MTLPrimitiveType type = ToMTLPrimitiveType(primitiveType);
    MTLIndexType mtlIndexType = ToMTLIndexType(indexType);
    
    [state.encoder drawIndexedPrimitives:type
                               indexCount:indexCount
                                indexType:mtlIndexType
                              indexBuffer:m_currentIndexBuffer->GetBuffer()
                        indexBufferOffset:indexOffset];
}

void RenderContextMTL::DrawArraysInstanced(PrimitiveType primitiveType, uint32_t vertexStart,
                                          uint32_t vertexCount, uint32_t instanceCount) {
    if (m_stateStack->IsEmpty()) {
        LR_LOG_ERROR_F("ContextMTL: DrawArraysInstanced called but no active render pass");
        return;
    }
    
    auto& state = m_stateStack->GetCurrentState();
    if (!state.encoder) {
        return;
    }
    
    MTLPrimitiveType type = ToMTLPrimitiveType(primitiveType);
    [state.encoder drawPrimitives:type
                       vertexStart:vertexStart
                       vertexCount:vertexCount
                     instanceCount:instanceCount];
}

void RenderContextMTL::DrawElementsInstanced(PrimitiveType primitiveType, uint32_t indexCount,
                                            IndexType indexType, size_t indexOffset,
                                            uint32_t instanceCount) {
    if (m_stateStack->IsEmpty()) {
        LR_LOG_ERROR_F("ContextMTL: DrawElementsInstanced called but no active render pass");
        return;
    }
    
    auto& state = m_stateStack->GetCurrentState();
    if (!state.encoder || !m_currentIndexBuffer) {
        return;
    }
    
    MTLPrimitiveType type = ToMTLPrimitiveType(primitiveType);
    MTLIndexType mtlIndexType = ToMTLIndexType(indexType);
    
    [state.encoder drawIndexedPrimitives:type
                               indexCount:indexCount
                                indexType:mtlIndexType
                              indexBuffer:m_currentIndexBuffer->GetBuffer()
                        indexBufferOffset:indexOffset
                            instanceCount:instanceCount];
}

// =============================================================================
// 同步操作
// =============================================================================

void RenderContextMTL::WaitIdle() {
    if (m_commandBufferPool) {
        m_commandBufferPool->WaitIdle();
    }
}

void RenderContextMTL::Flush() {
    // Metal的flush操作通过commit CommandBuffer实现
    // 在EndFrame时自动执行
}

// =============================================================================
// 帧控制
// =============================================================================

void RenderContextMTL::BeginFrame() {
    if (m_frameStarted) {
        LR_LOG_WARNING_F("ContextMTL: BeginFrame called but frame already started");
        return;
    }
    
    LR_LOG_TRACE_F("ContextMTL: BeginFrame");
    
    // 先设置标志
    m_frameStarted = true;
    
    // 获取下一个drawable
    if (m_metalLayer) {
        m_currentDrawable = [m_metalLayer nextDrawable];
        if (!m_currentDrawable) {
            LR_LOG_ERROR_F("ContextMTL: Failed to acquire drawable");
            m_frameStarted = false;
            return;
        }
        
        LR_LOG_TRACE_F("ContextMTL: Drawable acquired (%lux%lu)",
                    (unsigned long)m_currentDrawable.texture.width,
                    (unsigned long)m_currentDrawable.texture.height);
    }
    
    // 关键修复：为整个帧创建一个CommandBuffer
    if (m_commandBufferPool) {
        m_frameCommandBuffer = m_commandBufferPool->AcquireCommandBuffer();
        LR_LOG_INFO_F("ContextMTL: Frame CommandBuffer acquired: %p", m_frameCommandBuffer);
    }
}

void RenderContextMTL::EndFrame() {
    if (!m_frameStarted) {
        return;
    }
    
    LR_LOG_TRACE_F("ContextMTL: EndFrame");
    
    // 确保所有RenderPass都已结束
    if (!m_stateStack->IsEmpty()) {
        LR_LOG_WARNING_F("ContextMTL: EndFrame called with %u active render passes, ending them",
                       m_stateStack->GetDepth());
        while (!m_stateStack->IsEmpty()) {
            EndRenderPassEx();
        }
    }
    
    // Present drawable
    if (m_frameCommandBuffer && m_currentDrawable) {
        [m_frameCommandBuffer presentDrawable:m_currentDrawable];
        LR_LOG_INFO_F("ContextMTL: Presenting drawable");
    }
    
    // 提交CommandBuffer（通过池的SubmitCommandBuffer来注册completion handler）
    if (m_frameCommandBuffer && m_commandBufferPool) {
        LR_LOG_INFO_F("ContextMTL: Submitting Frame CommandBuffer: %p", m_frameCommandBuffer);
        m_commandBufferPool->SubmitCommandBuffer(m_frameCommandBuffer);
        m_frameCommandBuffer = nil;
    }
    
    m_currentDrawable = nil;
    m_frameStarted = false;
    
    LR_LOG_TRACE_F("ContextMTL: Frame ended");
}

// =============================================================================
// Metal特有方法
// =============================================================================

id<MTLCommandBuffer> RenderContextMTL::GetCurrentCommandBuffer() {
    if (!m_commandBufferPool) {
        LR_LOG_ERROR_F("ContextMTL: CommandBufferPool not initialized");
        return nil;
    }
    
    // 关键修复：复用帧级别的CommandBuffer
    if (m_frameCommandBuffer) {
        return m_frameCommandBuffer;
    }
    
    // 如果没有帧CommandBuffer，从池中获取新的
    LR_LOG_WARNING_F("ContextMTL: GetCurrentCommandBuffer called but no frame CommandBuffer");
    return m_commandBufferPool->AcquireCommandBuffer();
}

id<MTLRenderCommandEncoder> RenderContextMTL::GetCurrentRenderEncoder() {
    if (m_stateStack->IsEmpty()) {
        LR_LOG_WARNING_F("ContextMTL: GetCurrentRenderEncoder called but no active render pass");
        return nil;
    }
    
    auto& state = m_stateStack->GetCurrentState();
    return state.encoder;
}

void RenderContextMTL::SetVertexBuffer(VertexBufferMTL* buffer, uint32_t index) {
    m_currentVertexBuffer = buffer;
    
    if (!m_stateStack->IsEmpty()) {
        auto& state = m_stateStack->GetCurrentState();
        if (state.encoder && buffer) {
            [state.encoder setVertexBuffer:buffer->GetBuffer()
                                     offset:0
                                    atIndex:index];
        }
    }
}

void RenderContextMTL::SetIndexBuffer(IndexBufferMTL* buffer) {
    m_currentIndexBuffer = buffer;
}

void RenderContextMTL::SetPipelineState(PipelineStateMTL* pipelineState) {
    m_currentPipelineState = pipelineState;
    
    if (!m_stateStack->IsEmpty()) {
        auto& state = m_stateStack->GetCurrentState();
        if (state.encoder && pipelineState) {
            [state.encoder setRenderPipelineState:pipelineState->GetPipelineState()];
            [state.encoder setDepthStencilState:pipelineState->GetDepthStencilState()];
            [state.encoder setCullMode:pipelineState->GetCullMode()];
            [state.encoder setFrontFacingWinding:pipelineState->GetFrontFace()];
            [state.encoder setTriangleFillMode:pipelineState->GetFillMode()];
            
            if (pipelineState->GetDepthBias() != 0 || pipelineState->GetDepthBiasSlopeFactor() != 0) {
                [state.encoder setDepthBias:pipelineState->GetDepthBias()
                                  slopeScale:pipelineState->GetDepthBiasSlopeFactor()
                                       clamp:pipelineState->GetDepthBiasClamp()];
            }
        }
    }
}

void RenderContextMTL::SetUniformBuffer(UniformBufferMTL* buffer, uint32_t index) {
    if (!m_stateStack->IsEmpty()) {
        auto& state = m_stateStack->GetCurrentState();
        if (state.encoder && buffer) {
            LR_LOG_TRACE_F("ContextMTL: Binding uniform buffer to slot %u", index);
            
            // 关键修复：使用 setVertexBytes 而不是 setVertexBuffer
            // 这会让Metal复制一份数据，避免后续 UpdateData 覆盖
            void* contents = [buffer->GetBuffer() contents];
            size_t size = buffer->GetSize();
            
            if (contents && size > 0) {
                // 绑定到顶点着色器（复制数据）
                [state.encoder setVertexBytes:contents
                                        length:size
                                       atIndex:index];
                // 绑定到片段着色器（复制数据）
                [state.encoder setFragmentBytes:contents
                                          length:size
                                         atIndex:index];
                
                LR_LOG_TRACE_F("ContextMTL: Copied %zu bytes to encoder at index %u", size, index);
            }
        }
    }
}

void RenderContextMTL::SetTexture(TextureMTL* texture, uint32_t slot) {
    if (!m_stateStack->IsEmpty()) {
        auto& state = m_stateStack->GetCurrentState();
        if (state.encoder && texture) {
            id<MTLTexture> mtlTexture = (__bridge id<MTLTexture>)texture->GetNativeHandle().ptr;
            LR_LOG_INFO_F("ContextMTL: Binding texture %p (MTLTexture: %p) to slot %u", texture, mtlTexture, slot);
            
            // 绑定纹理到片段着色器
            [state.encoder setFragmentTexture:mtlTexture atIndex:slot];
            
            id<MTLSamplerState> samplerState = texture->GetSampler();
            // 如果Texture中没有采样器，创建默认采样器
            if (!samplerState) {
                MTLSamplerDescriptor* samplerDesc = [[MTLSamplerDescriptor alloc] init];
                samplerDesc.minFilter = MTLSamplerMinMagFilterLinear;
                samplerDesc.magFilter = MTLSamplerMinMagFilterLinear;
                samplerDesc.mipFilter = MTLSamplerMipFilterNotMipmapped;
                samplerDesc.sAddressMode = MTLSamplerAddressModeRepeat;
                samplerDesc.tAddressMode = MTLSamplerAddressModeRepeat;
                samplerDesc.rAddressMode = MTLSamplerAddressModeRepeat;
                
                samplerState = [m_device newSamplerStateWithDescriptor:samplerDesc];
                LR_LOG_INFO_F("ContextMTL: Created default sampler for texture");
            }

            [state.encoder setFragmentSamplerState:samplerState atIndex:slot];
        }
    }
}

// =============================================================================
// 渲染通道API（IRenderContextImpl接口）
// =============================================================================

void RenderContextMTL::BeginRenderPass(IFrameBufferImpl* frameBuffer) {
    LR_LOG_INFO_F("ContextMTL: BeginRenderPass (via interface) - frameBuffer: %p", frameBuffer);
    
    if (!m_frameStarted) {
        LR_LOG_ERROR_F("ContextMTL: BeginRenderPass called but frame not started");
        return;
    }
    
    if (frameBuffer) {
        // 离屏渲染：使用BeginOffscreenRenderPass
        FrameBufferMTL* framebufferMTL = static_cast<FrameBufferMTL*>(frameBuffer);
        BeginOffscreenRenderPass(framebufferMTL, nullptr);
    } else {
        // 默认framebuffer：使用BeginRenderPassEx(nullptr)
        BeginRenderPassEx(nullptr);
    }
}

void RenderContextMTL::EndRenderPass() {
    LR_LOG_INFO_F("ContextMTL: EndRenderPass (via interface)");
    EndRenderPassEx();
}

// =============================================================================
// 新渲染通道API实现
// =============================================================================

void RenderContextMTL::BeginRenderPassEx(RenderPassMTL* renderPass) {
    LR_LOG_INFO_F("ContextMTL: BeginRenderPassEx - renderPass: %p", renderPass);
    
    if (!m_frameStarted) {
        LR_LOG_ERROR_F("ContextMTL: BeginRenderPassEx called but frame not started");
        return;
    }
    
    // 如果renderPass为nullptr，创建默认RenderPass
    RenderPassMTL* activeRenderPass = renderPass;
    if (!activeRenderPass) {
        // 创建默认RenderPass配置
        RenderPassConfig config;
        config.useDefaultFrameBuffer = true;
        config.drawable = m_currentDrawable;
        config.defaultDepthTexture = m_depthStencilTexture;
        config.clearColor[0] = m_clearColor[0];
        config.clearColor[1] = m_clearColor[1];
        config.clearColor[2] = m_clearColor[2];
        config.clearColor[3] = m_clearColor[3];
        config.clearDepth = m_clearDepth;
        
        m_defaultRenderPass = std::make_unique<RenderPassMTL>(config);
        activeRenderPass = m_defaultRenderPass.get();
    }
    
    // 创建encoder
    id<MTLRenderCommandEncoder> encoder = CreateRenderEncoder(activeRenderPass);
    if (!encoder) {
        LR_LOG_ERROR_F("ContextMTL: Failed to create render encoder");
        return;
    }
    
    // 保存状态
    RenderState state;
    state.renderPass = activeRenderPass;
    state.encoder = encoder;
    state.commandBuffer = GetCurrentCommandBuffer();
    state.viewport = m_viewport;
    state.scissor = m_scissorRect;
    state.pipelineState = m_currentPipelineState;
    
    m_stateStack->PushState(state);
    
    // 应用视口和裁剪
    ApplyViewportAndScissor();
    
    // 恢复管线状态
    RestorePipelineState();
    
    LR_LOG_INFO_F("ContextMTL: RenderPass started (stack depth: %u)", m_stateStack->GetDepth());
}

void RenderContextMTL::EndRenderPassEx() {
    if (m_stateStack->IsEmpty()) {
        LR_LOG_WARNING_F("ContextMTL: EndRenderPassEx called but no active render pass");
        return;
    }
    
    // 弹出状态
    RenderState state = m_stateStack->PopState();
    
    // 结束encoder
    if (state.encoder) {
        [state.encoder endEncoding];
        LR_LOG_INFO_F("ContextMTL: Render encoder ended");
    }
    
    LR_LOG_INFO_F("ContextMTL: RenderPass ended (stack depth: %u)", m_stateStack->GetDepth());
}

void RenderContextMTL::BeginMRTRenderPass(const std::vector<FrameBufferMTL*>& colorTargets,
                                         FrameBufferMTL* depthTarget) {
    // 注意：此方法保留以兼容旧代码，但新设计中使用单个FrameBuffer
    LR_LOG_INFO_F("ContextMTL: BeginMRTRenderPass - %zu color targets (legacy)", colorTargets.size());
    
    if (colorTargets.empty()) {
        LR_LOG_ERROR_F("ContextMTL: BeginMRTRenderPass requires at least one color target");
        return;
    }
    
    // 使用第一个FrameBuffer（新设计中应该只传一个）
    FrameBufferMTL* frameBuffer = colorTargets[0];
    if (colorTargets.size() > 1) {
        LR_LOG_WARNING_F("ContextMTL: Multiple FrameBuffers passed, only using the first one. "
                        "Use single FrameBuffer with multiple color attachments for MRT.");
    }
    
    // 创建RenderPass配置
    RenderPassConfig config;
    config.frameBuffer = frameBuffer;
    config.clearColor[0] = m_clearColor[0];
    config.clearColor[1] = m_clearColor[1];
    config.clearColor[2] = m_clearColor[2];
    config.clearColor[3] = m_clearColor[3];
    config.clearDepth = m_clearDepth;
    
    m_currentRenderPass = std::make_unique<RenderPassMTL>(config);
    
    BeginRenderPassEx(m_currentRenderPass.get());
}

void RenderContextMTL::BeginOffscreenRenderPass(FrameBufferMTL* target, FrameBufferMTL* depthTarget) {
    if (!target) {
        LR_LOG_ERROR_F("ContextMTL: BeginOffscreenRenderPass requires a valid target");
        return;
    }
    
    LR_LOG_INFO_F("ContextMTL: BeginOffscreenRenderPass - target: %p", target);
    
    // 直接使用单个FrameBuffer
    std::vector<FrameBufferMTL*> targets = {target};
    BeginMRTRenderPass(targets, depthTarget);
}

// =============================================================================
// 内部辅助方法
// =============================================================================

void RenderContextMTL::CreateDepthStencilTexture() {
    if (m_width == 0 || m_height == 0) {
        return;
    }
    
    MTLTextureDescriptor* depthDesc = [[MTLTextureDescriptor alloc] init];
    depthDesc.textureType = MTLTextureType2D;
    depthDesc.pixelFormat = MTLPixelFormatDepth32Float;
    depthDesc.width = m_width;
    depthDesc.height = m_height;
    depthDesc.usage = MTLTextureUsageRenderTarget;
    depthDesc.storageMode = MTLStorageModePrivate;
    
    m_depthStencilTexture = [m_device newTextureWithDescriptor:depthDesc];
    m_depthStencilTexture.label = @"Default Depth Texture";
}

id<MTLRenderCommandEncoder> RenderContextMTL::CreateRenderEncoder(RenderPassMTL* renderPass) {
    if (!renderPass) {
        LR_LOG_ERROR_F("ContextMTL: CreateRenderEncoder requires a valid renderPass");
        return nil;
    }
    
    id<MTLCommandBuffer> commandBuffer = GetCurrentCommandBuffer();
    if (!commandBuffer) {
        LR_LOG_ERROR_F("ContextMTL: No command buffer available");
        return nil;
    }
    
    MTLRenderPassDescriptor* passDesc = renderPass->CreateDescriptor();
    if (!passDesc) {
        LR_LOG_ERROR_F("ContextMTL: Failed to create render pass descriptor");
        return nil;
    }
    
    id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:passDesc];
    if (!encoder) {
        LR_LOG_ERROR_F("ContextMTL: Failed to create render encoder");
        return nil;
    }
    
    LR_LOG_INFO_F("ContextMTL: Render encoder created (%ux%u)", 
                renderPass->GetWidth(), renderPass->GetHeight());
    
    return encoder;
}

void RenderContextMTL::EndCurrentEncoder() {
    if (!m_stateStack->IsEmpty()) {
        auto& state = m_stateStack->GetCurrentState();
        if (state.encoder) {
            [state.encoder endEncoding];
            state.encoder = nil;
            LR_LOG_INFO_F("ContextMTL: Current encoder ended");
        }
    }
}

void RenderContextMTL::ApplyViewportAndScissor() {
    if (m_stateStack->IsEmpty()) {
        return;
    }
    
    auto& state = m_stateStack->GetCurrentState();
    if (state.encoder) {
        [state.encoder setViewport:m_viewport];
        [state.encoder setScissorRect:m_scissorRect];
    }
}

void RenderContextMTL::RestorePipelineState() {
    if (m_stateStack->IsEmpty()) {
        return;
    }
    
    auto& state = m_stateStack->GetCurrentState();
    if (!state.encoder || !m_currentPipelineState) {
        return;
    }
    
    if (m_currentPipelineState->GetPipelineState()) {
        [state.encoder setRenderPipelineState:m_currentPipelineState->GetPipelineState()];
    }
    
    if (m_currentPipelineState->GetDepthStencilState()) {
        [state.encoder setDepthStencilState:m_currentPipelineState->GetDepthStencilState()];
    }
    
    [state.encoder setCullMode:m_currentPipelineState->GetCullMode()];
    [state.encoder setFrontFacingWinding:m_currentPipelineState->GetFrontFace()];
    [state.encoder setTriangleFillMode:m_currentPipelineState->GetFillMode()];
}

} // namespace mtl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
