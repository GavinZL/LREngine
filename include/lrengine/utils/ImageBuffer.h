#pragma once

#include <lrengine/core/LRTypes.h>
#include <cstdint>
#include <memory>

namespace lrengine {

// 将 render 命名空间的类型引入当前作用域
using render::ImageFormat;
using render::ImageDataDesc;
using render::ImagePlaneDesc;
using render::ColorSpace;
using render::ColorRange;

namespace utils {

/**
 * @brief 图像缓冲区抽象基类
 * 
 * 封装平台特定的图像内存对象（CVPixelBuffer、HardwareBuffer、CPU 内存等），
 * 提供统一的访问接口。
 */
class ImageBuffer {
public:
    virtual ~ImageBuffer() = default;

    /**
     * @brief 获取图像数据描述
     */
    virtual const ImageDataDesc& GetImageDesc() const = 0;

    /**
     * @brief 获取平台原生缓冲区句柄
     * @return void* 指向平台特定对象
     */
    virtual void* GetNativeBuffer() const = 0;

    /**
     * @brief 锁定缓冲区以进行 CPU 访问
     * @param readOnly 是否只读访问
     * @return 是否成功锁定
     */
    virtual bool Lock(bool readOnly = true) = 0;

    /**
     * @brief 解锁缓冲区
     */
    virtual void Unlock() = 0;

    /**
     * @brief 检查缓冲区是否当前被锁定
     */
    virtual bool IsLocked() const = 0;

    /**
     * @brief 获取缓冲区类型标识
     */
    enum class BufferType {
        HostMemory,         ///< CPU 主机内存
        CVPixelBuffer,      ///< iOS/macOS CVPixelBuffer
        HardwareBuffer,     ///< Android HardwareBuffer
        Unknown
    };
    virtual BufferType GetBufferType() const = 0;

protected:
    ImageBuffer() = default;
    ImageBuffer(const ImageBuffer&) = delete;
    ImageBuffer& operator=(const ImageBuffer&) = delete;
};

/**
 * @brief CPU 主机内存实现
 */
class HostMemoryBuffer : public ImageBuffer {
public:
    /**
     * @brief 构造函数
     * @param imageDesc 图像描述
     * @param allocate 是否立即分配内存
     */
    explicit HostMemoryBuffer(const ImageDataDesc& imageDesc, bool allocate = true);
    ~HostMemoryBuffer() override;

    // ImageBuffer 接口实现
    const ImageDataDesc& GetImageDesc() const override { return mImageDesc; }
    void* GetNativeBuffer() const override { return mData.get(); }
    bool Lock(bool readOnly = true) override;
    void Unlock() override;
    bool IsLocked() const override { return mIsLocked; }
    BufferType GetBufferType() const override { return BufferType::HostMemory; }

    /**
     * @brief 获取指定平面的数据指针
     * @param planeIndex 平面索引
     * @return 平面数据指针
     */
    void* GetPlaneData(int planeIndex) const;

private:
    void AllocateMemory();
    void FreeMemory();

    ImageDataDesc mImageDesc;
    std::unique_ptr<uint8_t[]> mData;   ///< 实际内存
    std::vector<size_t> mPlaneOffsets;  ///< 各平面在内存中的偏移量
    bool mIsLocked = false;
};

#ifdef __APPLE__
/**
 * @brief iOS/macOS CVPixelBuffer 包装器
 */
class CVPixelBufferWrapper : public ImageBuffer {
public:
    /**
     * @brief 从现有 CVPixelBuffer 创建包装器
     * @param pixelBuffer CVPixelBuffer 对象（会增加引用计数）
     */
    explicit CVPixelBufferWrapper(void* pixelBuffer);
    
    /**
     * @brief 创建新的 CVPixelBuffer
     * @param imageDesc 图像描述
     */
    explicit CVPixelBufferWrapper(const ImageDataDesc& imageDesc);
    
    ~CVPixelBufferWrapper() override;

    // ImageBuffer 接口实现
    const ImageDataDesc& GetImageDesc() const override { return mImageDesc; }
    void* GetNativeBuffer() const override { return mPixelBuffer; }
    bool Lock(bool readOnly = true) override;
    void Unlock() override;
    bool IsLocked() const override { return mIsLocked; }
    BufferType GetBufferType() const override { return BufferType::CVPixelBuffer; }

private:
    void ExtractImageDesc();
    void CreatePixelBuffer();

    void* mPixelBuffer = nullptr;       ///< CVPixelBufferRef
    ImageDataDesc mImageDesc;
    bool mIsLocked = false;
};
#endif // __APPLE__

#ifdef __ANDROID__
/**
 * @brief Android HardwareBuffer 包装器
 */
class HardwareBufferWrapper : public ImageBuffer {
public:
    /**
     * @brief 从现有 AHardwareBuffer 创建包装器
     * @param hardwareBuffer AHardwareBuffer 对象（会增加引用计数）
     */
    explicit HardwareBufferWrapper(void* hardwareBuffer);
    
    /**
     * @brief 创建新的 AHardwareBuffer
     * @param imageDesc 图像描述
     */
    explicit HardwareBufferWrapper(const ImageDataDesc& imageDesc);
    
    ~HardwareBufferWrapper() override;

    // ImageBuffer 接口实现
    const ImageDataDesc& GetImageDesc() const override { return mImageDesc; }
    void* GetNativeBuffer() const override { return mHardwareBuffer; }
    bool Lock(bool readOnly = true) override;
    void Unlock() override;
    bool IsLocked() const override { return mIsLocked; }
    BufferType GetBufferType() const override { return BufferType::HardwareBuffer; }

private:
    void ExtractImageDesc();
    void CreateHardwareBuffer();

    void* mHardwareBuffer = nullptr;    ///< AHardwareBuffer*
    ImageDataDesc mImageDesc;
    bool mIsLocked = false;
};
#endif // __ANDROID__

} // namespace utils
} // namespace lrengine
