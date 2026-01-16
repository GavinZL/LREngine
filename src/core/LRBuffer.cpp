/**
 * @file LRBuffer.cpp
 * @brief LREngine缓冲区实现
 */

#include "lrengine/core/LRBuffer.h"
#include "lrengine/core/LRError.h"
#include "platform/interface/IBufferImpl.h"

namespace lrengine {
namespace render {

// =============================================================================
// LRBuffer
// =============================================================================

LRBuffer::LRBuffer()
    : LRResource(ResourceType::Buffer) {
}

LRBuffer::~LRBuffer() {
    if (mImpl) {
        mImpl->Destroy();
        delete mImpl;
        mImpl = nullptr;
    }
}

bool LRBuffer::Initialize(IBufferImpl* impl, const BufferDescriptor& desc) {
    if (!impl) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Buffer implementation is null");
        return false;
    }
    
    mImpl = impl;
    
    if (!mImpl->Create(desc)) {
        LR_SET_ERROR(ErrorCode::ResourceCreationFailed, "Failed to create buffer");
        delete mImpl;
        mImpl = nullptr;
        return false;
    }
    
    mSize = desc.size;
    mUsage = desc.usage;
    mBufferType = desc.type;
    mIsValid = true;
    
    if (desc.debugName) {
        SetDebugName(desc.debugName);
    }
    
    return true;
}

void LRBuffer::UpdateData(const void* data, size_t size, size_t offset) {
    if (!mImpl || !mIsValid) {
        LR_SET_ERROR(ErrorCode::ResourceInvalid, "Buffer is not valid");
        return;
    }
    
    if (offset + size > mSize) {
        LR_SET_ERROR(ErrorCode::BufferTooSmall, "Update size exceeds buffer size");
        return;
    }
    
    mImpl->UpdateData(data, size, offset);
}

void* LRBuffer::Map(MemoryAccess access) {
    if (!mImpl || !mIsValid) {
        LR_SET_ERROR(ErrorCode::ResourceInvalid, "Buffer is not valid");
        return nullptr;
    }
    
    return mImpl->Map(access);
}

void LRBuffer::Unmap() {
    if (mImpl && mIsValid) {
        mImpl->Unmap();
    }
}

void LRBuffer::Bind() {
    if (mImpl && mIsValid) {
        mImpl->Bind();
    }
}

void LRBuffer::Unbind() {
    if (mImpl && mIsValid) {
        mImpl->Unbind();
    }
}

ResourceHandle LRBuffer::GetNativeHandle() const {
    if (mImpl) {
        return mImpl->GetNativeHandle();
    }
    return ResourceHandle();
}

// =============================================================================
// LRVertexBuffer
// =============================================================================

LRVertexBuffer::LRVertexBuffer()
    : LRBuffer() {
    mResourceType = ResourceType::VertexBuffer;
    mBufferType = BufferType::Vertex;
}

bool LRVertexBuffer::Initialize(IBufferImpl* impl, const BufferDescriptor& desc) {
    BufferDescriptor vertexDesc = desc;
    vertexDesc.type = BufferType::Vertex;
    return LRBuffer::Initialize(impl, vertexDesc);
}

void LRVertexBuffer::SetVertexLayout(const VertexLayoutDescriptor& layout) {
    mVertexLayout = layout;
    
    // 将布局传递给平台实现
    if (mImpl) {
        mImpl->SetVertexLayout(layout);
    }
}

uint32_t LRVertexBuffer::GetVertexCount() const {
    if (mVertexLayout.stride > 0) {
        return static_cast<uint32_t>(mSize / mVertexLayout.stride);
    }
    return 0;
}

// =============================================================================
// LRIndexBuffer
// =============================================================================

LRIndexBuffer::LRIndexBuffer()
    : LRBuffer() {
    mResourceType = ResourceType::IndexBuffer;
    mBufferType = BufferType::Index;
}

bool LRIndexBuffer::Initialize(IBufferImpl* impl, const BufferDescriptor& desc) {
    BufferDescriptor indexDesc = desc;
    indexDesc.type = BufferType::Index;
    return LRBuffer::Initialize(impl, indexDesc);
}

uint32_t LRIndexBuffer::GetIndexCount() const {
    size_t indexSize = (mIndexType == IndexType::UInt16) ? 2 : 4;
    return static_cast<uint32_t>(mSize / indexSize);
}

// =============================================================================
// LRUniformBuffer
// =============================================================================

LRUniformBuffer::LRUniformBuffer()
    : LRBuffer() {
    mResourceType = ResourceType::UniformBuffer;
    mBufferType = BufferType::Uniform;
}

bool LRUniformBuffer::Initialize(IBufferImpl* impl, const BufferDescriptor& desc) {
    BufferDescriptor uniformDesc = desc;
    uniformDesc.type = BufferType::Uniform;
    return LRBuffer::Initialize(impl, uniformDesc);
}

void LRUniformBuffer::BindToPoint(uint32_t point) {
    mBindingPoint = point;
    // 实际绑定由平台实现处理
    Bind();
}

} // namespace render
} // namespace lrengine
