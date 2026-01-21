/**
 * @file LRResource.cpp
 * @brief LREngine渲染资源基类实现
 */

#include "lrengine/core/LRResource.h"

#include <atomic>

namespace lrengine {
namespace render {

namespace {
// 全局资源ID计数器
std::atomic<uint64_t> s_resourceIDCounter {1};
} // namespace

LRResource::LRResource(ResourceType type)
    : mResourceID(GenerateResourceID()), mResourceType(type), mRefCount(1), mIsValid(false) {}

LRResource::~LRResource() {
    // 派生类应该在析构前释放资源
}

LRResource::LRResource(LRResource&& other) noexcept
    : mResourceID(other.mResourceID)
    , mResourceType(other.mResourceType)
    , mRefCount(other.mRefCount.load())
    , mDebugName(std::move(other.mDebugName))
    , mIsValid(other.mIsValid) {
    other.mResourceID = 0;
    other.mIsValid    = false;
}

LRResource& LRResource::operator=(LRResource&& other) noexcept {
    if (this != &other) {
        mResourceID   = other.mResourceID;
        mResourceType = other.mResourceType;
        mRefCount.store(other.mRefCount.load());
        mDebugName = std::move(other.mDebugName);
        mIsValid   = other.mIsValid;

        other.mResourceID = 0;
        other.mIsValid    = false;
    }
    return *this;
}

void LRResource::AddRef() { mRefCount.fetch_add(1, std::memory_order_relaxed); }

void LRResource::Release() {
    if (mRefCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        delete this;
    }
}

uint32_t LRResource::GetRefCount() const { return mRefCount.load(std::memory_order_relaxed); }

bool LRResource::IsValid() const { return mIsValid && GetNativeHandle().IsValid(); }

void LRResource::SetDebugName(const char* name) { mDebugName = name ? name : ""; }

uint64_t LRResource::GenerateResourceID() {
    return s_resourceIDCounter.fetch_add(1, std::memory_order_relaxed);
}

} // namespace render
} // namespace lrengine
