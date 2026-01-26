#include <lrengine/utils/ImageBufferPool.h>
#include <chrono>
#include <algorithm>

namespace lrengine {
namespace utils {

// =============================================================================
// ImageBufferPool 实现
// =============================================================================

ImageBufferPool::ImageBufferPool()
    : mOptions() {
    // 预留空间
    mBuffers.reserve(mOptions.maxPoolSize);
}

ImageBufferPool::ImageBufferPool(const PoolOptions& options)
    : mOptions(options) {
    // 预留空间
    mBuffers.reserve(mOptions.maxPoolSize);
}

ImageBufferPool::~ImageBufferPool() {
    Clear();
}

ImageBufferPool::ImageBufferPtr ImageBufferPool::Acquire(const ImageDataDesc& imageDesc) {
    std::lock_guard<std::mutex> lock(mMutex);

    // 查找可用的兼容缓冲区
    ImageBuffer* buffer = FindAvailableBuffer(imageDesc);

    // 如果没有找到，尝试创建新缓冲区
    if (!buffer && (mOptions.autoGrow || mBuffers.size() < mOptions.maxPoolSize)) {
        buffer = CreateNewBuffer(imageDesc);
    }

    if (buffer) {
        // 标记为使用中
        for (auto& entry : mBuffers) {
            if (entry.buffer.get() == buffer) {
                entry.inUse = true;
                entry.lastUsedTime = GetCurrentTimeMs();
                mInUseCount++;
                break;
            }
        }

        // 返回带自定义删除器的智能指针
        return ImageBufferPtr(buffer, BufferDeleter(this));
    }

    return nullptr;
}

void ImageBufferPool::Release(ImageBuffer* buffer) {
    if (!buffer) {
        return;
    }

    std::lock_guard<std::mutex> lock(mMutex);

    // 查找并标记为可用
    bool found = false;
    for (auto& entry : mBuffers) {
        if (entry.buffer.get() == buffer) {
            entry.inUse = false;
            entry.lastUsedTime = GetCurrentTimeMs();
            mInUseCount--;
            found = true;
            break;
        }
    }

    if (!found) {
        // 这不应该发生，说明有问题
        return;
    }

    // 如果池过大，清理一些未使用的缓冲区
    if (mBuffers.size() > mOptions.maxPoolSize) {
        TrimExcessBuffers();
    }
}

void ImageBufferPool::Clear() {
    std::lock_guard<std::mutex> lock(mMutex);
    
    // 只清除未使用的缓冲区
    auto it = mBuffers.begin();
    while (it != mBuffers.end()) {
        if (!it->inUse) {
            it = mBuffers.erase(it);
        } else {
            ++it;
        }
    }
    
    // 如果所有缓冲区都被清除了，重置计数
    if (mBuffers.empty()) {
        mInUseCount = 0;
    }
}

size_t ImageBufferPool::GetAvailableCount() const {
    std::lock_guard<std::mutex> lock(mMutex);
    size_t count = 0;
    for (const auto& entry : mBuffers) {
        if (!entry.inUse) {
            count++;
        }
    }
    return count;
}

size_t ImageBufferPool::GetInUseCount() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mInUseCount;
}

size_t ImageBufferPool::GetCapacity() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mBuffers.size();
}

void ImageBufferPool::SetMaxPoolSize(size_t maxSize) {
    std::lock_guard<std::mutex> lock(mMutex);
    mOptions.maxPoolSize = maxSize;
    
    if (mBuffers.size() > maxSize) {
        TrimExcessBuffers();
    }
}

void ImageBufferPool::Preallocate(const ImageDataDesc& imageDesc, size_t count) {
    std::lock_guard<std::mutex> lock(mMutex);

    for (size_t i = 0; i < count; ++i) {
        if (mBuffers.size() >= mOptions.maxPoolSize) {
            break;
        }
        CreateNewBuffer(imageDesc);
    }
}

// =============================================================================
// 私有辅助方法
// =============================================================================

bool ImageBufferPool::IsCompatible(const ImageBuffer* buffer, const ImageDataDesc& desc) const {
    if (!buffer) {
        return false;
    }

    const auto& bufferDesc = buffer->GetImageDesc();
    
    // 检查尺寸和格式是否匹配
    return (bufferDesc.width == desc.width &&
            bufferDesc.height == desc.height &&
            bufferDesc.format == desc.format);
}

ImageBuffer* ImageBufferPool::FindAvailableBuffer(const ImageDataDesc& desc) {
    for (auto& entry : mBuffers) {
        if (!entry.inUse && IsCompatible(entry.buffer.get(), desc)) {
            return entry.buffer.get();
        }
    }
    return nullptr;
}

ImageBuffer* ImageBufferPool::CreateNewBuffer(const ImageDataDesc& desc) {
    if (mBuffers.size() >= mOptions.maxPoolSize) {
        return nullptr;
    }

    std::unique_ptr<ImageBuffer> buffer;

    // 根据配置的缓冲区类型创建
    switch (mOptions.preferredType) {
        case ImageBuffer::BufferType::HostMemory:
            buffer = std::make_unique<HostMemoryBuffer>(desc, true);
            break;

#ifdef __APPLE__
        case ImageBuffer::BufferType::CVPixelBuffer:
            buffer = std::make_unique<CVPixelBufferWrapper>(desc);
            break;
#endif

#ifdef __ANDROID__
        case ImageBuffer::BufferType::HardwareBuffer:
            buffer = std::make_unique<HardwareBufferWrapper>(desc);
            break;
#endif

        default:
            // 默认使用主机内存
            buffer = std::make_unique<HostMemoryBuffer>(desc, true);
            break;
    }

    if (buffer) {
        ImageBuffer* bufferPtr = buffer.get();
        mBuffers.emplace_back(std::move(buffer));
        return bufferPtr;
    }

    return nullptr;
}

void ImageBufferPool::TrimExcessBuffers() {
    // 如果池大小未超过限制，不需要清理
    if (mBuffers.size() <= mOptions.maxPoolSize) {
        return;
    }

    // 计算需要移除的数量
    size_t removeCount = mBuffers.size() - mOptions.maxPoolSize;

    // 按最后使用时间排序（LRU 策略）
    std::vector<size_t> unusedIndices;
    for (size_t i = 0; i < mBuffers.size(); ++i) {
        if (!mBuffers[i].inUse) {
            unusedIndices.push_back(i);
        }
    }

    // 按最后使用时间排序
    std::sort(unusedIndices.begin(), unusedIndices.end(),
              [this](size_t a, size_t b) {
                  return mBuffers[a].lastUsedTime < mBuffers[b].lastUsedTime;
              });

    // 移除最旧的未使用缓冲区
    size_t removed = 0;
    for (size_t idx : unusedIndices) {
        if (removed >= removeCount) {
            break;
        }
        // 注意：这里需要从后向前删除以避免索引失效
        // 简化处理：标记为需要删除，然后统一删除
        mBuffers[idx].buffer.reset();
        removed++;
    }

    // 移除所有标记为删除的条目
    auto it = mBuffers.begin();
    while (it != mBuffers.end()) {
        if (!it->buffer) {
            it = mBuffers.erase(it);
        } else {
            ++it;
        }
    }
}

uint64_t ImageBufferPool::GetCurrentTimeMs() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch());
    return static_cast<uint64_t>(duration.count());
}

} // namespace utils
} // namespace lrengine
