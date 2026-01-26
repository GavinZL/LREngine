#pragma once

#include <lrengine/utils/ImageBuffer.h>
#include <memory>
#include <mutex>
#include <vector>
#include <functional>

namespace lrengine {
namespace utils {

/**
 * @brief 图像缓冲区对象池
 * 
 * 用于高效管理和复用 ImageBuffer 对象，减少内存分配开销。
 * 支持按格式和尺寸进行池化管理。
 */
class ImageBufferPool {
public:
    /**
     * @brief 池配置选项
     */
    struct PoolOptions {
        size_t maxPoolSize = 10;                    ///< 最大池容量
        size_t initialPoolSize = 3;                 ///< 初始池大小
        bool autoGrow = true;                       ///< 是否自动增长
        ImageBuffer::BufferType preferredType =     ///< 优先使用的缓冲区类型
            ImageBuffer::BufferType::HostMemory;
        
        PoolOptions() = default;
    };

    /**
     * @brief 构造函数
     * @param options 池配置选项
     */
    ImageBufferPool();
    explicit ImageBufferPool(const PoolOptions& options);
    ~ImageBufferPool();

    /**
     * @brief 从池中获取一个图像缓冲区
     * @param imageDesc 图像描述
     * @return 图像缓冲区智能指针（自动归还到池中）
     */
    using ImageBufferPtr = std::shared_ptr<ImageBuffer>;
    ImageBufferPtr Acquire(const ImageDataDesc& imageDesc);

    /**
     * @brief 手动归还缓冲区到池中
     * @param buffer 要归还的缓冲区
     * @note 通常不需要手动调用，智能指针会自动归还
     */
    void Release(ImageBuffer* buffer);

    /**
     * @brief 清空池中的所有缓冲区
     */
    void Clear();

    /**
     * @brief 获取池中可用缓冲区数量
     */
    size_t GetAvailableCount() const;

    /**
     * @brief 获取池中正在使用的缓冲区数量
     */
    size_t GetInUseCount() const;

    /**
     * @brief 获取池的总容量
     */
    size_t GetCapacity() const;

    /**
     * @brief 设置最大池容量
     */
    void SetMaxPoolSize(size_t maxSize);

    /**
     * @brief 预分配指定格式和尺寸的缓冲区
     * @param imageDesc 图像描述
     * @param count 预分配数量
     */
    void Preallocate(const ImageDataDesc& imageDesc, size_t count);

private:
    // 缓冲区包装结构
    struct BufferEntry {
        std::unique_ptr<ImageBuffer> buffer;
        bool inUse = false;
        uint64_t lastUsedTime = 0;  // 用于 LRU 策略
        
        BufferEntry() = default;
        explicit BufferEntry(std::unique_ptr<ImageBuffer> buf)
            : buffer(std::move(buf)), inUse(false), lastUsedTime(0) {}
    };

    // 辅助方法
    bool IsCompatible(const ImageBuffer* buffer, const ImageDataDesc& desc) const;
    ImageBuffer* FindAvailableBuffer(const ImageDataDesc& desc);
    ImageBuffer* CreateNewBuffer(const ImageDataDesc& desc);
    void TrimExcessBuffers();
    uint64_t GetCurrentTimeMs() const;

    // 自定义删除器，用于智能指针归还缓冲区
    class BufferDeleter {
    public:
        explicit BufferDeleter(ImageBufferPool* pool) : mPool(pool) {}
        void operator()(ImageBuffer* buffer) const {
            if (mPool && buffer) {
                mPool->Release(buffer);
            }
        }
    private:
        ImageBufferPool* mPool;
    };

    PoolOptions mOptions;
    std::vector<BufferEntry> mBuffers;
    mutable std::mutex mMutex;
    size_t mInUseCount = 0;
};

/**
 * @brief 格式化的图像缓冲区池（模板辅助类）
 * 
 * 为特定格式和尺寸提供专用的缓冲区池，简化使用。
 */
template<ImageFormat FormatType>
class TypedImageBufferPool {
public:
    using ImageBufferPtr = typename ImageBufferPool::ImageBufferPtr;

    TypedImageBufferPool(uint32_t width, uint32_t height)
        : mWidth(width), mHeight(height), mPool() {
        // 默认构造，不预分配
    }

    /**
     * @brief 构造函数
     * @param width 图像宽度
     * @param height 图像高度
     * @param options 池配置选项
     */
    TypedImageBufferPool(uint32_t width, uint32_t height,
                        const ImageBufferPool::PoolOptions& options)
        : mWidth(width), mHeight(height), mPool(options) {
        // 预分配初始缓冲区
        if (options.initialPoolSize > 0) {
            ImageDataDesc desc = CreateImageDesc();
            mPool.Preallocate(desc, options.initialPoolSize);
        }
    }

    /**
     * @brief 获取一个缓冲区
     */
    ImageBufferPtr Acquire() {
        return mPool.Acquire(CreateImageDesc());
    }

    /**
     * @brief 清空池
     */
    void Clear() {
        mPool.Clear();
    }

    /**
     * @brief 获取池统计信息
     */
    size_t GetAvailableCount() const { return mPool.GetAvailableCount(); }
    size_t GetInUseCount() const { return mPool.GetInUseCount(); }
    size_t GetCapacity() const { return mPool.GetCapacity(); }

private:
    ImageDataDesc CreateImageDesc() const {
        ImageDataDesc desc;
        desc.width = mWidth;
        desc.height = mHeight;
        desc.format = FormatType;
        desc.colorSpace = ColorSpace::BT709;
        desc.range = ColorRange::Video;
        return desc;
    }

    uint32_t mWidth;
    uint32_t mHeight;
    ImageBufferPool mPool;
};

// 常用类型的别名
using RGBA8BufferPool = TypedImageBufferPool<ImageFormat::RGBA8>;
using NV12BufferPool = TypedImageBufferPool<ImageFormat::NV12>;
using YUV420PBufferPool = TypedImageBufferPool<ImageFormat::YUV420P>;

} // namespace utils
} // namespace lrengine
