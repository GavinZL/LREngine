/**
 * @file LRRenderContext.cpp
 * @brief LREngine渲染上下文实现
 */

#include "lrengine/core/LRRenderContext.h"
#include "lrengine/core/LRBuffer.h"
#include "lrengine/core/LRShader.h"
#include "lrengine/core/LRTexture.h"
#include "lrengine/core/LRFrameBuffer.h"
#include "lrengine/core/LRPipelineState.h"
#include "lrengine/core/LRFence.h"
#include "lrengine/core/LRError.h"
#include "lrengine/utils/LRLog.h"
#include "lrengine/factory/LRDeviceFactory.h"
#include "platform/interface/IRenderContextImpl.h"
#include "platform/interface/IBufferImpl.h"
#include "platform/interface/IShaderImpl.h"
#include "platform/interface/ITextureImpl.h"
#include "platform/interface/IFrameBufferImpl.h"
#include "platform/interface/IPipelineStateImpl.h"
#include "platform/interface/IFenceImpl.h"

namespace lrengine {
namespace render {

LRRenderContext::LRRenderContext() = default;

LRRenderContext::~LRRenderContext() {
    Shutdown();
}

LRRenderContext* LRRenderContext::Create(const RenderContextDescriptor& desc) {
    LRRenderContext* context = new LRRenderContext();
    
    if (!context->Initialize(desc)) {
        delete context;
        return nullptr;
    }
    
    return context;
}

void LRRenderContext::Destroy(LRRenderContext* context) {
    delete context;
}

bool LRRenderContext::Initialize(const RenderContextDescriptor& desc) {
    LR_LOG_INFO_F("LRRenderContext::Initialize: backend=%d, size=%ux%u", (int)desc.backend, desc.width, desc.height);
    // 获取设备工厂
    LRDeviceFactory* factory = LRDeviceFactory::GetFactory(desc.backend);
    if (!factory) {
        LR_SET_ERROR(ErrorCode::BackendNotAvailable, "Backend not available");
        return false;
    }
    
    // 创建渲染上下文实现
    mImpl = factory->CreateRenderContextImpl();
    if (!mImpl) {
        LR_SET_ERROR(ErrorCode::ContextCreationFailed, "Failed to create render context implementation");
        return false;
    }
    
    // 初始化
    if (!mImpl->Initialize(desc)) {
        LR_SET_ERROR(ErrorCode::ContextCreationFailed, "Failed to initialize render context");
        delete mImpl;
        mImpl = nullptr;
        return false;
    }
    
    mBackend = desc.backend;
    mWidth = desc.width;
    mHeight = desc.height;
    
    return true;
}

void LRRenderContext::Shutdown() {
    if (mImpl) {
        mImpl->Shutdown();
        delete mImpl;
        mImpl = nullptr;
    }
}

// =============================================================================
// 资源创建
// =============================================================================

LRBuffer* LRRenderContext::CreateBuffer(const BufferDescriptor& desc) {
    if (!mImpl) {
        LR_SET_ERROR(ErrorCode::NotInitialized, "Render context not initialized");
        return nullptr;
    }
    
    IBufferImpl* impl = mImpl->CreateBufferImpl();
    if (!impl) {
        return nullptr;
    }
    
    LRBuffer* buffer = new LRBuffer();
    if (!buffer->Initialize(impl, desc)) {
        delete buffer;
        return nullptr;
    }
    
    return buffer;
}

LRVertexBuffer* LRRenderContext::CreateVertexBuffer(const BufferDescriptor& desc) {
    if (!mImpl) {
        LR_SET_ERROR(ErrorCode::NotInitialized, "Render context not initialized");
        return nullptr;
    }
    
    IBufferImpl* impl = mImpl->CreateBufferImpl(BufferType::Vertex);
    if (!impl) {
        return nullptr;
    }
    
    LRVertexBuffer* buffer = new LRVertexBuffer();
    if (!buffer->Initialize(impl, desc)) {
        delete buffer;
        return nullptr;
    }
    
    return buffer;
}

LRIndexBuffer* LRRenderContext::CreateIndexBuffer(const BufferDescriptor& desc) {
    if (!mImpl) {
        LR_SET_ERROR(ErrorCode::NotInitialized, "Render context not initialized");
        return nullptr;
    }
    
    IBufferImpl* impl = mImpl->CreateBufferImpl(BufferType::Index);
    if (!impl) {
        return nullptr;
    }
    
    LRIndexBuffer* buffer = new LRIndexBuffer();
    if (!buffer->Initialize(impl, desc)) {
        delete buffer;
        return nullptr;
    }
    
    return buffer;
}

LRUniformBuffer* LRRenderContext::CreateUniformBuffer(const BufferDescriptor& desc) {
    if (!mImpl) {
        LR_SET_ERROR(ErrorCode::NotInitialized, "Render context not initialized");
        return nullptr;
    }
    
    IBufferImpl* impl = mImpl->CreateBufferImpl(BufferType::Uniform);
    if (!impl) {
        return nullptr;
    }
    
    LRUniformBuffer* buffer = new LRUniformBuffer();
    if (!buffer->Initialize(impl, desc)) {
        delete buffer;
        return nullptr;
    }
    
    return buffer;
}

LRShader* LRRenderContext::CreateShader(const ShaderDescriptor& desc) {
    if (!mImpl) {
        LR_SET_ERROR(ErrorCode::NotInitialized, "Render context not initialized");
        return nullptr;
    }
    
    IShaderImpl* impl = mImpl->CreateShaderImpl();
    if (!impl) {
        return nullptr;
    }
    
    LRShader* shader = new LRShader();
    if (!shader->Initialize(impl, desc)) {
        delete shader;
        return nullptr;
    }
    
    return shader;
}

LRShaderProgram* LRRenderContext::CreateShaderProgram(LRShader* vertexShader,
                                                     LRShader* fragmentShader,
                                                     LRShader* geometryShader) {
    if (!mImpl) {
        LR_SET_ERROR(ErrorCode::NotInitialized, "Render context not initialized");
        return nullptr;
    }
    
    IShaderProgramImpl* impl = mImpl->CreateShaderProgramImpl();
    if (!impl) {
        return nullptr;
    }
    
    LRShaderProgram* program = new LRShaderProgram();
    if (!program->Initialize(impl, vertexShader, fragmentShader, geometryShader)) {
        delete program;
        return nullptr;
    }
    
    return program;
}

LRTexture* LRRenderContext::CreateTexture(const TextureDescriptor& desc) {
    if (!mImpl) {
        LR_SET_ERROR(ErrorCode::NotInitialized, "Render context not initialized");
        return nullptr;
    }
    
    ITextureImpl* impl = mImpl->CreateTextureImpl();
    if (!impl) {
        return nullptr;
    }
    
    LRTexture* texture = new LRTexture();
    if (!texture->Initialize(impl, desc)) {
        delete texture;
        return nullptr;
    }
    
    return texture;
}

LRFrameBuffer* LRRenderContext::CreateFrameBuffer(const FrameBufferDescriptor& desc) {
    if (!mImpl) {
        LR_SET_ERROR(ErrorCode::NotInitialized, "Render context not initialized");
        return nullptr;
    }
    
    IFrameBufferImpl* impl = mImpl->CreateFrameBufferImpl();
    if (!impl) {
        return nullptr;
    }
    
    LRFrameBuffer* frameBuffer = new LRFrameBuffer();
    if (!frameBuffer->Initialize(impl, desc)) {
        delete frameBuffer;
        return nullptr;
    }
    
    return frameBuffer;
}

LRPipelineState* LRRenderContext::CreatePipelineState(const PipelineStateDescriptor& desc) {
    if (!mImpl) {
        LR_SET_ERROR(ErrorCode::NotInitialized, "Render context not initialized");
        return nullptr;
    }
    
    // 首先创建着色器程序（如果提供了着色器）
    LRShaderProgram* program = nullptr;
    if (desc.vertexShader && desc.fragmentShader) {
        program = CreateShaderProgram(desc.vertexShader, desc.fragmentShader, desc.geometryShader);
        if (!program) {
            return nullptr;
        }
    }
    
    IPipelineStateImpl* impl = mImpl->CreatePipelineStateImpl();
    if (!impl) {
        if (program) program->Release();
        return nullptr;
    }
    
    LRPipelineState* pipelineState = new LRPipelineState();
    if (!pipelineState->Initialize(impl, desc, program)) {
        if (program) program->Release();
        delete pipelineState;
        return nullptr;
    }
    
    return pipelineState;
}

LRFence* LRRenderContext::CreateFence() {
    if (!mImpl) {
        LR_SET_ERROR(ErrorCode::NotInitialized, "Render context not initialized");
        return nullptr;
    }
    
    IFenceImpl* impl = mImpl->CreateFenceImpl();
    if (!impl) {
        return nullptr;
    }
    
    LRFence* fence = new LRFence();
    if (!fence->Initialize(impl)) {
        delete fence;
        return nullptr;
    }
    
    return fence;
}

// =============================================================================
// 帧控制
// =============================================================================

void LRRenderContext::BeginFrame() {
    if (mImpl) {
        mImpl->BeginFrame();
    }
}

void LRRenderContext::EndFrame() {
    if (mImpl) {
        mImpl->EndFrame();
    }
}

void LRRenderContext::Present() {
    if (mImpl) {
        mImpl->SwapBuffers();
    }
}

// =============================================================================
// 渲染目标
// =============================================================================

void LRRenderContext::BeginRenderPass(LRFrameBuffer* frameBuffer) {
    mCurrentFrameBuffer = frameBuffer;
    
    // 调用后端实现（Metal后端会使用此方法创建渲染通道）
    if (mImpl) {
        IFrameBufferImpl* fbImpl = frameBuffer ? frameBuffer->GetImpl() : nullptr;
        mImpl->BeginRenderPass(fbImpl);
    }
    
    // 注意：不再调用frameBuffer->Bind()，避免在Metal后端重复创建渲染通道
    // OpenGL等后端在自己的BeginRenderPass实现中处理Bind逻辑
}

void LRRenderContext::EndRenderPass() {
    // 调用后端实现
    if (mImpl) {
        mImpl->EndRenderPass();
    }
    
    // 注意：不再调用frameBuffer->Unbind()，避免在Metal后端重复操作
    mCurrentFrameBuffer = nullptr;
}

// =============================================================================
// 渲染状态
// =============================================================================

void LRRenderContext::SetViewport(int32_t x, int32_t y, int32_t width, int32_t height) {
    if (mImpl) {
        mImpl->SetViewport(x, y, width, height);
    }
}

void LRRenderContext::SetScissor(int32_t x, int32_t y, int32_t width, int32_t height) {
    if (mImpl) {
        mImpl->SetScissor(x, y, width, height);
    }
}

void LRRenderContext::SetPipelineState(LRPipelineState* pipelineState) {
    LR_LOG_TRACE_F("LRRenderContext::SetPipelineState: %p", pipelineState);
    mCurrentPipelineState = pipelineState;
    
    if (pipelineState) {
        pipelineState->Apply();
        mCurrentPrimitiveType = pipelineState->GetPrimitiveType();
        
        // 使用着色器程序
        if (pipelineState->GetShaderProgram()) {
            pipelineState->GetShaderProgram()->Use();
        }
        
        // 通知后端绑定管线状态
        if (mImpl && pipelineState->GetImpl()) {
            mImpl->BindPipelineState(pipelineState->GetImpl());
        }
    }
}

void LRRenderContext::SetVertexBuffer(LRVertexBuffer* buffer, uint32_t slot) {
    LR_LOG_TRACE_F("LRRenderContext::SetVertexBuffer: %p, slot=%u", buffer, slot);
    if (buffer) {
        buffer->Bind();
        
        // 通知后端绑定顶点缓冲区
        if (mImpl && buffer->GetImpl()) {
            mImpl->BindVertexBuffer(buffer->GetImpl(), slot);
        }
    }
}

void LRRenderContext::SetIndexBuffer(LRIndexBuffer* buffer) {
    if (buffer) {
        buffer->Bind();
        mCurrentIndexType = buffer->GetIndexType();
        
        // 通知后端绑定索引缓冲区
        if (mImpl && buffer->GetImpl()) {
            mImpl->BindIndexBuffer(buffer->GetImpl());
        }
    }
}

void LRRenderContext::SetUniformBuffer(LRUniformBuffer* buffer, uint32_t slot) {
    if (buffer) {
        buffer->SetBindingPoint(slot);
        buffer->Bind();
        // 通知后端实现进行实际绑定
        if (mImpl) {
            mImpl->BindUniformBuffer(buffer->GetImpl(), slot);
        }
    }
}

void LRRenderContext::SetTexture(LRTexture* texture, uint32_t slot) {
    LR_LOG_TRACE_F("LRRenderContext::SetTexture: %p, slot=%u", texture, slot);
    if (texture) {
        texture->Bind(slot);
        // 通知后端实现进行实际绑定
        if (mImpl) {
            mImpl->BindTexture(texture->GetImpl(), slot);
        }
    }
}

// =============================================================================
// 清除
// =============================================================================

void LRRenderContext::ClearColor(float r, float g, float b, float a) {
    float color[4] = {r, g, b, a};
    if (mImpl) {
        mImpl->Clear(ClearFlag_Color, color, 1.0f, 0);
    }
}

void LRRenderContext::ClearDepth(float depth) {
    if (mImpl) {
        mImpl->Clear(ClearFlag_Depth, nullptr, depth, 0);
    }
}

void LRRenderContext::ClearStencil(uint8_t stencil) {
    if (mImpl) {
        mImpl->Clear(ClearFlag_Stencil, nullptr, 1.0f, stencil);
    }
}

void LRRenderContext::Clear(uint8_t flags, float r, float g, float b, float a, float depth, uint8_t stencil) {
    LR_LOG_TRACE("LRRenderContext::Clear");
    float color[4] = {r, g, b, a};
    if (mImpl) {
        mImpl->Clear(flags, color, depth, stencil);
    }
}

// =============================================================================
// 绘制
// =============================================================================

void LRRenderContext::Draw(uint32_t vertexStart, uint32_t vertexCount) {
    LR_LOG_TRACE_F("LRRenderContext::Draw: start=%u, count=%u", vertexStart, vertexCount);
    if (mImpl) {
        mImpl->DrawArrays(mCurrentPrimitiveType, vertexStart, vertexCount);
    }
}

void LRRenderContext::DrawIndexed(uint32_t indexStart, uint32_t indexCount) {
    if (mImpl) {
        size_t offset = indexStart * (mCurrentIndexType == IndexType::UInt16 ? 2 : 4);
        mImpl->DrawElements(mCurrentPrimitiveType, indexCount, mCurrentIndexType, offset);
    }
}

void LRRenderContext::DrawInstanced(uint32_t vertexStart, uint32_t vertexCount, uint32_t instanceCount) {
    if (mImpl) {
        mImpl->DrawArraysInstanced(mCurrentPrimitiveType, vertexStart, vertexCount, instanceCount);
    }
}

void LRRenderContext::DrawIndexedInstanced(uint32_t indexStart, uint32_t indexCount, uint32_t instanceCount) {
    if (mImpl) {
        size_t offset = indexStart * (mCurrentIndexType == IndexType::UInt16 ? 2 : 4);
        mImpl->DrawElementsInstanced(mCurrentPrimitiveType, indexCount, mCurrentIndexType, offset, instanceCount);
    }
}

// =============================================================================
// 同步
// =============================================================================

void LRRenderContext::WaitIdle() {
    if (mImpl) {
        mImpl->WaitIdle();
    }
}

void LRRenderContext::Flush() {
    if (mImpl) {
        mImpl->Flush();
    }
}

void LRRenderContext::MakeCurrent() {
    if (mImpl) {
        mImpl->MakeCurrent();
    }
}

} // namespace render
} // namespace lrengine
