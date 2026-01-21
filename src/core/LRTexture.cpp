/**
 * @file LRTexture.cpp
 * @brief LREngine纹理实现
 */

#include "lrengine/core/LRTexture.h"
#include "lrengine/core/LRError.h"
#include "platform/interface/ITextureImpl.h"

namespace lrengine {
namespace render {

LRTexture::LRTexture() : LRResource(ResourceType::Texture) {}

LRTexture::~LRTexture() {
    if (mImpl) {
        mImpl->Destroy();
        delete mImpl;
        mImpl = nullptr;
    }
}

bool LRTexture::Initialize(ITextureImpl* impl, const TextureDescriptor& desc) {
    if (!impl) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Texture implementation is null");
        return false;
    }

    mImpl = impl;

    if (!mImpl->Create(desc)) {
        LR_SET_ERROR(ErrorCode::TextureCreationFailed, "Failed to create texture");
        delete mImpl;
        mImpl = nullptr;
        return false;
    }

    mWidth       = desc.width;
    mHeight      = desc.height;
    mDepth       = desc.depth;
    mTextureType = desc.type;
    mFormat      = desc.format;
    mMipLevels   = desc.mipLevels;
    mSamples     = desc.sampleCount;
    mIsValid     = true;

    if (desc.debugName) {
        SetDebugName(desc.debugName);
    }

    return true;
}

void LRTexture::UpdateData(const void* data, const TextureRegion* region) {
    if (!mImpl || !mIsValid) {
        LR_SET_ERROR(ErrorCode::ResourceInvalid, "Texture is not valid");
        return;
    }

    mImpl->UpdateData(data, 0, region);
}

void LRTexture::GenerateMipmaps() {
    if (mImpl && mIsValid) {
        mImpl->GenerateMipmaps();
    }
}

void LRTexture::Bind(uint32_t slot) {
    mBoundSlot = slot;
    if (mImpl && mIsValid) {
        mImpl->Bind(slot);
    }
}

void LRTexture::Unbind() {
    if (mImpl && mIsValid) {
        mImpl->Unbind(mBoundSlot);
    }
}

ResourceHandle LRTexture::GetNativeHandle() const {
    if (mImpl) {
        return mImpl->GetNativeHandle();
    }
    return ResourceHandle();
}

} // namespace render
} // namespace lrengine
