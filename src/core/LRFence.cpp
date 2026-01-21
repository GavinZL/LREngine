/**
 * @file LRFence.cpp
 * @brief LREngine同步栅栏实现
 */

#include "lrengine/core/LRFence.h"
#include "lrengine/core/LRError.h"
#include "platform/interface/IFenceImpl.h"

namespace lrengine {
namespace render {

LRFence::LRFence() : LRResource(ResourceType::Fence) {}

LRFence::~LRFence() {
    if (mImpl) {
        mImpl->Destroy();
        delete mImpl;
        mImpl = nullptr;
    }
}

bool LRFence::Initialize(IFenceImpl* impl) {
    if (!impl) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Fence implementation is null");
        return false;
    }

    mImpl = impl;

    if (!mImpl->Create()) {
        LR_SET_ERROR(ErrorCode::ResourceCreationFailed, "Failed to create fence");
        delete mImpl;
        mImpl = nullptr;
        return false;
    }

    mIsValid = true;
    return true;
}

void LRFence::Signal() {
    if (mImpl && mIsValid) {
        mImpl->Signal();
    }
}

bool LRFence::Wait(uint64_t timeoutNs) {
    if (!mImpl || !mIsValid) {
        LR_SET_ERROR(ErrorCode::ResourceInvalid, "Fence is not valid");
        return false;
    }

    return mImpl->Wait(timeoutNs);
}

FenceStatus LRFence::GetStatus() const {
    if (!mImpl || !mIsValid) {
        return FenceStatus::Error;
    }

    return mImpl->GetStatus();
}

void LRFence::Reset() {
    if (mImpl && mIsValid) {
        mImpl->Reset();
    }
}

ResourceHandle LRFence::GetNativeHandle() const {
    if (mImpl) {
        return mImpl->GetNativeHandle();
    }
    return ResourceHandle();
}

} // namespace render
} // namespace lrengine
