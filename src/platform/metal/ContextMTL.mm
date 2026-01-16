/**
 * @file ContextMTL.mm
 * @brief Metal渲染上下文实现
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
#include "lrengine/core/LRError.h"

#import <Cocoa/Cocoa.h>

namespace lrengine {
namespace render {
namespace mtl {

RenderContextMTL::RenderContextMTL()
    : m_device(nil)
    , m_commandQueue(nil)
    , m_currentCommandBuffer(nil)
    , m_currentRenderEncoder(nil)
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
        
        [contentView setLayer:m_metalLayer];
    }

    // 创建深度/模板纹理
    CreateDepthStencilTexture();

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

    return true;
}

void RenderContextMTL::Shutdown() {
    EndCurrentRenderEncoder();
    
    if (m_currentCommandBuffer) {
        [m_currentCommandBuffer commit];
        [m_currentCommandBuffer waitUntilCompleted];
        m_currentCommandBuffer = nil;
    }
    
    m_depthStencilTexture = nil;
    m_currentDrawable = nil;
    m_metalLayer = nil;
    m_commandQueue = nil;
    m_device = nil;
}

void RenderContextMTL::MakeCurrent() {
    // Metal没有上下文切换的概念
}

void RenderContextMTL::SwapBuffers() {
    EndFrame();
    BeginFrame();
}

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
    return new FrameBufferMTL(m_device);
}

IPipelineStateImpl* RenderContextMTL::CreatePipelineStateImpl() {
    return new PipelineStateMTL(m_device);
}

IFenceImpl* RenderContextMTL::CreateFenceImpl() {
    return new FenceMTL(m_device);
}

void RenderContextMTL::SetViewport(int32_t x, int32_t y, int32_t width, int32_t height) {
    m_viewport.originX = x;
    m_viewport.originY = y;
    m_viewport.width = width;
    m_viewport.height = height;
    m_viewportDirty = true;
    
    if (m_currentRenderEncoder) {
        [m_currentRenderEncoder setViewport:m_viewport];
    }
}

void RenderContextMTL::SetScissor(int32_t x, int32_t y, int32_t width, int32_t height) {
    m_scissorRect.x = x;
    m_scissorRect.y = y;
    m_scissorRect.width = width;
    m_scissorRect.height = height;
    m_scissorDirty = true;
    
    if (m_currentRenderEncoder) {
        [m_currentRenderEncoder setScissorRect:m_scissorRect];
    }
}

void RenderContextMTL::Clear(uint8_t flags, const float* color, float depth, uint8_t stencil) {
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
    
    // 结束当前渲染编码器，下次绘制时会用新的清除值创建
    EndCurrentRenderEncoder();
}

void RenderContextMTL::BindPipelineState(IPipelineStateImpl* pipelineState) {
    m_currentPipelineState = static_cast<PipelineStateMTL*>(pipelineState);
    
    if (m_currentRenderEncoder && m_currentPipelineState) {
        if (m_currentPipelineState->GetPipelineState()) {
            [m_currentRenderEncoder setRenderPipelineState:m_currentPipelineState->GetPipelineState()];
        }
        if (m_currentPipelineState->GetDepthStencilState()) {
            [m_currentRenderEncoder setDepthStencilState:m_currentPipelineState->GetDepthStencilState()];
        }
        [m_currentRenderEncoder setCullMode:m_currentPipelineState->GetCullMode()];
        [m_currentRenderEncoder setFrontFacingWinding:m_currentPipelineState->GetFrontFace()];
        [m_currentRenderEncoder setTriangleFillMode:m_currentPipelineState->GetFillMode()];
    }
}

void RenderContextMTL::BindVertexBuffer(IBufferImpl* buffer, uint32_t slot) {
    m_currentVertexBuffer = static_cast<VertexBufferMTL*>(buffer);
    
    if (m_currentRenderEncoder && m_currentVertexBuffer && m_currentVertexBuffer->GetBuffer()) {
        [m_currentRenderEncoder setVertexBuffer:m_currentVertexBuffer->GetBuffer()
                                         offset:0
                                        atIndex:slot];
    }
}

void RenderContextMTL::BindIndexBuffer(IBufferImpl* buffer) {
    m_currentIndexBuffer = static_cast<IndexBufferMTL*>(buffer);
}

void RenderContextMTL::DrawArrays(PrimitiveType primitiveType, uint32_t vertexStart, uint32_t vertexCount) {
    EnsureRenderEncoder();
    
    if (!m_currentRenderEncoder) {
        return;
    }
    
    MTLPrimitiveType type = ToMTLPrimitiveType(primitiveType);
    [m_currentRenderEncoder drawPrimitives:type
                               vertexStart:vertexStart
                               vertexCount:vertexCount];
}

void RenderContextMTL::DrawElements(PrimitiveType primitiveType, uint32_t indexCount,
                                   IndexType indexType, size_t indexOffset) {
    EnsureRenderEncoder();
    
    if (!m_currentRenderEncoder || !m_currentIndexBuffer) {
        return;
    }
    
    MTLPrimitiveType type = ToMTLPrimitiveType(primitiveType);
    MTLIndexType mtlIndexType = ToMTLIndexType(indexType);
    
    [m_currentRenderEncoder drawIndexedPrimitives:type
                                       indexCount:indexCount
                                        indexType:mtlIndexType
                                      indexBuffer:m_currentIndexBuffer->GetBuffer()
                                indexBufferOffset:indexOffset];
}

void RenderContextMTL::DrawArraysInstanced(PrimitiveType primitiveType, uint32_t vertexStart,
                                          uint32_t vertexCount, uint32_t instanceCount) {
    EnsureRenderEncoder();
    
    if (!m_currentRenderEncoder) {
        return;
    }
    
    MTLPrimitiveType type = ToMTLPrimitiveType(primitiveType);
    [m_currentRenderEncoder drawPrimitives:type
                               vertexStart:vertexStart
                               vertexCount:vertexCount
                             instanceCount:instanceCount];
}

void RenderContextMTL::DrawElementsInstanced(PrimitiveType primitiveType, uint32_t indexCount,
                                            IndexType indexType, size_t indexOffset,
                                            uint32_t instanceCount) {
    EnsureRenderEncoder();
    
    if (!m_currentRenderEncoder || !m_currentIndexBuffer) {
        return;
    }
    
    MTLPrimitiveType type = ToMTLPrimitiveType(primitiveType);
    MTLIndexType mtlIndexType = ToMTLIndexType(indexType);
    
    [m_currentRenderEncoder drawIndexedPrimitives:type
                                       indexCount:indexCount
                                        indexType:mtlIndexType
                                      indexBuffer:m_currentIndexBuffer->GetBuffer()
                                indexBufferOffset:indexOffset
                                    instanceCount:instanceCount];
}

void RenderContextMTL::WaitIdle() {
    EndCurrentRenderEncoder();
    
    if (m_currentCommandBuffer) {
        [m_currentCommandBuffer commit];
        [m_currentCommandBuffer waitUntilCompleted];
        m_currentCommandBuffer = nil;
    }
}

void RenderContextMTL::Flush() {
    EndCurrentRenderEncoder();
    
    if (m_currentCommandBuffer) {
        [m_currentCommandBuffer commit];
        m_currentCommandBuffer = nil;
    }
}

id<MTLCommandBuffer> RenderContextMTL::GetCurrentCommandBuffer() {
    if (!m_currentCommandBuffer) {
        m_currentCommandBuffer = [m_commandQueue commandBuffer];
    }
    return m_currentCommandBuffer;
}

id<MTLRenderCommandEncoder> RenderContextMTL::GetCurrentRenderEncoder() {
    EnsureRenderEncoder();
    return m_currentRenderEncoder;
}

void RenderContextMTL::BeginFrame() {
    if (m_frameStarted) {
        return;
    }
    
    // 获取下一个drawable
    if (m_metalLayer) {
        m_currentDrawable = [m_metalLayer nextDrawable];
    }
    
    m_frameStarted = true;
}

void RenderContextMTL::EndFrame() {
    if (!m_frameStarted) {
        return;
    }
    
    EndCurrentRenderEncoder();
    
    if (m_currentCommandBuffer && m_currentDrawable) {
        [m_currentCommandBuffer presentDrawable:m_currentDrawable];
        [m_currentCommandBuffer commit];
        m_currentCommandBuffer = nil;
    }
    
    m_currentDrawable = nil;
    m_frameStarted = false;
}

void RenderContextMTL::SetVertexBuffer(VertexBufferMTL* buffer, uint32_t index) {
    m_currentVertexBuffer = buffer;
    
    if (m_currentRenderEncoder && buffer) {
        [m_currentRenderEncoder setVertexBuffer:buffer->GetBuffer()
                                         offset:0
                                        atIndex:index];
    }
}

void RenderContextMTL::SetIndexBuffer(IndexBufferMTL* buffer) {
    m_currentIndexBuffer = buffer;
}

void RenderContextMTL::SetPipelineState(PipelineStateMTL* pipelineState) {
    m_currentPipelineState = pipelineState;
    
    if (m_currentRenderEncoder && pipelineState) {
        [m_currentRenderEncoder setRenderPipelineState:pipelineState->GetPipelineState()];
        [m_currentRenderEncoder setDepthStencilState:pipelineState->GetDepthStencilState()];
        [m_currentRenderEncoder setCullMode:pipelineState->GetCullMode()];
        [m_currentRenderEncoder setFrontFacingWinding:pipelineState->GetFrontFace()];
        [m_currentRenderEncoder setTriangleFillMode:pipelineState->GetFillMode()];
        
        if (pipelineState->GetDepthBias() != 0 || pipelineState->GetDepthBiasSlopeFactor() != 0) {
            [m_currentRenderEncoder setDepthBias:pipelineState->GetDepthBias()
                                      slopeScale:pipelineState->GetDepthBiasSlopeFactor()
                                           clamp:pipelineState->GetDepthBiasClamp()];
        }
    }
}

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

void RenderContextMTL::EnsureRenderEncoder() {
    if (m_currentRenderEncoder) {
        return;
    }
    
    if (!m_frameStarted) {
        BeginFrame();
    }
    
    // 确保有drawable
    if (!m_currentDrawable && m_metalLayer) {
        m_currentDrawable = [m_metalLayer nextDrawable];
    }
    
    if (!m_currentDrawable) {
        // 没有可用的drawable，无法创建渲染编码器
        return;
    }
    
    id<MTLCommandBuffer> commandBuffer = GetCurrentCommandBuffer();
    if (!commandBuffer) {
        return;
    }
    
    // 创建渲染通道描述符
    MTLRenderPassDescriptor* passDesc = [[MTLRenderPassDescriptor alloc] init];
    
    // 设置颜色附件
    passDesc.colorAttachments[0].texture = m_currentDrawable.texture;
    passDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
    passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
    passDesc.colorAttachments[0].clearColor = MTLClearColorMake(m_clearColor[0], m_clearColor[1],
                                                                 m_clearColor[2], m_clearColor[3]);
    
    // 设置深度附件
    if (m_depthStencilTexture) {
        passDesc.depthAttachment.texture = m_depthStencilTexture;
        passDesc.depthAttachment.loadAction = MTLLoadActionClear;
        passDesc.depthAttachment.storeAction = MTLStoreActionDontCare;
        passDesc.depthAttachment.clearDepth = m_clearDepth;
    }
    
    // 创建渲染命令编码器
    m_currentRenderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:passDesc];
    
    if (!m_currentRenderEncoder) {
        return;
    }
    
    // 设置视口和裁剪
    [m_currentRenderEncoder setViewport:m_viewport];
    [m_currentRenderEncoder setScissorRect:m_scissorRect];
    
    // 恢复管线状态
    if (m_currentPipelineState && m_currentPipelineState->GetPipelineState()) {
        [m_currentRenderEncoder setRenderPipelineState:m_currentPipelineState->GetPipelineState()];
        if (m_currentPipelineState->GetDepthStencilState()) {
            [m_currentRenderEncoder setDepthStencilState:m_currentPipelineState->GetDepthStencilState()];
        }
        [m_currentRenderEncoder setCullMode:m_currentPipelineState->GetCullMode()];
        [m_currentRenderEncoder setFrontFacingWinding:m_currentPipelineState->GetFrontFace()];
        [m_currentRenderEncoder setTriangleFillMode:m_currentPipelineState->GetFillMode()];
    }
    
    // 恢复顶点缓冲区
    if (m_currentVertexBuffer && m_currentVertexBuffer->GetBuffer()) {
        [m_currentRenderEncoder setVertexBuffer:m_currentVertexBuffer->GetBuffer()
                                         offset:0
                                        atIndex:0];
    }
}

void RenderContextMTL::EndCurrentRenderEncoder() {
    if (m_currentRenderEncoder) {
        [m_currentRenderEncoder endEncoding];
        m_currentRenderEncoder = nil;
    }
}

} // namespace mtl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
