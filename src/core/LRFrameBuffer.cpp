/**
 * @file LRFrameBuffer.cpp
 * @brief LREngine帧缓冲实现
 */

#include "lrengine/core/LRFrameBuffer.h"
#include "lrengine/core/LRTexture.h"
#include "lrengine/core/LRError.h"
#include "platform/interface/IFrameBufferImpl.h"
#include "platform/interface/ITextureImpl.h"

namespace lrengine {
namespace render {

LRFrameBuffer::LRFrameBuffer() : LRResource(ResourceType::FrameBuffer) {}

LRFrameBuffer::~LRFrameBuffer() {
    if (mImpl) {
        mImpl->Destroy();
        delete mImpl;
        mImpl = nullptr;
    }
}

bool LRFrameBuffer::Initialize(IFrameBufferImpl* impl, const FrameBufferDescriptor& desc) {
    if (!impl) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "FrameBuffer implementation is null");
        return false;
    }

    mImpl = impl;

    if (!mImpl->Create(desc)) {
        LR_SET_ERROR(ErrorCode::ResourceCreationFailed, "Failed to create framebuffer");
        delete mImpl;
        mImpl = nullptr;
        return false;
    }

    mWidth   = desc.width;
    mHeight  = desc.height;
    mIsValid = true;

    if (desc.debugName) {
        SetDebugName(desc.debugName);
    }

    return true;
}

void LRFrameBuffer::AttachColorTexture(LRTexture* texture, uint32_t index) {
    if (!mImpl || !mIsValid) {
        LR_SET_ERROR(ErrorCode::ResourceInvalid, "FrameBuffer is not valid");
        return;
    }

    if (index >= 8) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Color attachment index out of range");
        return;
    }

    // 确保vector足够大
    if (mColorTextures.size() <= index) {
        mColorTextures.resize(index + 1, nullptr);
    }

    mColorTextures[index] = texture;

    // 将纹理附加到平台实现
    mImpl->AttachColorTexture(texture ? texture->GetImpl() : nullptr, index);
}

void LRFrameBuffer::AttachDepthTexture(LRTexture* texture) {
    if (!mImpl || !mIsValid) {
        LR_SET_ERROR(ErrorCode::ResourceInvalid, "FrameBuffer is not valid");
        return;
    }

    mDepthTexture = texture;

    // 将深度纹理附加到平台实现
    mImpl->AttachDepthTexture(texture ? texture->GetImpl() : nullptr);
}

void LRFrameBuffer::AttachStencilTexture(LRTexture* texture) {
    if (!mImpl || !mIsValid) {
        LR_SET_ERROR(ErrorCode::ResourceInvalid, "FrameBuffer is not valid");
        return;
    }

    mStencilTexture = texture;
}

bool LRFrameBuffer::IsComplete() const {
    if (!mImpl || !mIsValid) {
        return false;
    }
    return mImpl->IsComplete();
}

void LRFrameBuffer::Bind() {
    if (mImpl && mIsValid) {
        mImpl->Bind();
    }
}

void LRFrameBuffer::Unbind() {
    if (mImpl && mIsValid) {
        mImpl->Unbind();
    }
}

void LRFrameBuffer::Clear(uint8_t flags, const float* color, float depth, uint8_t stencil) {
    if (mImpl && mIsValid) {
        mImpl->Clear(flags, color, depth, stencil);
    }
}

LRTexture* LRFrameBuffer::GetColorTexture(uint32_t index) const {
    if (index < mColorTextures.size()) {
        return mColorTextures[index];
    }
    return nullptr;
}

ResourceHandle LRFrameBuffer::GetNativeHandle() const {
    if (mImpl) {
        return mImpl->GetNativeHandle();
    }
    return ResourceHandle();
}

} // namespace render
} // namespace lrengine
