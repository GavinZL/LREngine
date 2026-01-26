#include <lrengine/utils/ImageBuffer.h>
#include <cstring>
#include <algorithm>

#ifdef __APPLE__
#include <CoreVideo/CoreVideo.h>
#endif

#ifdef __ANDROID__
#include <android/hardware_buffer.h>
#endif

namespace lrengine {
namespace utils {

// =============================================================================
// HostMemoryBuffer 实现
// =============================================================================

namespace {
    // 计算指定格式单个平面所需的内存大小
    size_t CalculatePlaneSize(ImageFormat format, uint32_t width, uint32_t height, int planeIndex) {
        switch (format) {
            case ImageFormat::YUV420P:
                if (planeIndex == 0) {
                    return width * height; // Y 平面
                } else {
                    return (width / 2) * (height / 2); // U/V 平面
                }
            case ImageFormat::NV12:
            case ImageFormat::NV21:
                if (planeIndex == 0) {
                    return width * height; // Y 平面
                } else {
                    return width * (height / 2); // UV 交织平面
                }
            case ImageFormat::RGBA8:
            case ImageFormat::BGRA8:
                return width * height * 4;
            case ImageFormat::RGB8:
                return width * height * 3;
            case ImageFormat::GRAY8:
                return width * height;
            default:
                return 0;
        }
    }

    // 获取指定格式的平面数量
    int GetPlaneCount(ImageFormat format) {
        switch (format) {
            case ImageFormat::YUV420P:
                return 3; // Y, U, V
            case ImageFormat::NV12:
            case ImageFormat::NV21:
                return 2; // Y, UV
            case ImageFormat::RGBA8:
            case ImageFormat::BGRA8:
            case ImageFormat::RGB8:
            case ImageFormat::GRAY8:
                return 1;
            default:
                return 0;
        }
    }
}

HostMemoryBuffer::HostMemoryBuffer(const ImageDataDesc& imageDesc, bool allocate)
    : mImageDesc(imageDesc) {
    if (allocate) {
        AllocateMemory();
    }
}

HostMemoryBuffer::~HostMemoryBuffer() {
    FreeMemory();
}

void HostMemoryBuffer::AllocateMemory() {
    FreeMemory();

    int planeCount = GetPlaneCount(mImageDesc.format);
    if (planeCount == 0) {
        return;
    }

    // 计算总内存大小和各平面偏移
    size_t totalSize = 0;
    mPlaneOffsets.resize(planeCount);

    for (int i = 0; i < planeCount; ++i) {
        mPlaneOffsets[i] = totalSize;
        uint32_t stride = 0;
        if (i < static_cast<int>(mImageDesc.planes.size())) {
            stride = mImageDesc.planes[i].stride;
        }
        
        // 如果没有指定 stride，计算默认值
        if (stride == 0) {
            uint32_t width = mImageDesc.width;
            if (i > 0 && (mImageDesc.format == ImageFormat::YUV420P ||
                         mImageDesc.format == ImageFormat::NV12 ||
                         mImageDesc.format == ImageFormat::NV21)) {
                width = mImageDesc.width / 2;
            }
            switch (mImageDesc.format) {
                case ImageFormat::RGBA8:
                case ImageFormat::BGRA8:
                    stride = width * 4;
                    break;
                case ImageFormat::RGB8:
                    stride = width * 3;
                    break;
                default:
                    stride = width;
                    break;
            }
        }

        uint32_t height = mImageDesc.height;
        if (i > 0 && (mImageDesc.format == ImageFormat::YUV420P ||
                     mImageDesc.format == ImageFormat::NV12 ||
                     mImageDesc.format == ImageFormat::NV21)) {
            height = mImageDesc.height / 2;
        }

        totalSize += stride * height;
    }

    // 分配内存
    mData = std::make_unique<uint8_t[]>(totalSize);
    std::memset(mData.get(), 0, totalSize);

    // 更新 ImageDataDesc 中的平面描述
    mImageDesc.planes.resize(planeCount);
    for (int i = 0; i < planeCount; ++i) {
        mImageDesc.planes[i].data = mData.get() + mPlaneOffsets[i];
        // stride 已在上面计算过，这里需要重新设置
    }
}

void HostMemoryBuffer::FreeMemory() {
    mData.reset();
    mPlaneOffsets.clear();
    mImageDesc.planes.clear();
}

bool HostMemoryBuffer::Lock(bool readOnly) {
    (void)readOnly; // CPU 内存无需真正锁定
    mIsLocked = true;
    return true;
}

void HostMemoryBuffer::Unlock() {
    mIsLocked = false;
}

void* HostMemoryBuffer::GetPlaneData(int planeIndex) const {
    if (planeIndex < 0 || planeIndex >= static_cast<int>(mPlaneOffsets.size())) {
        return nullptr;
    }
    return mData.get() + mPlaneOffsets[planeIndex];
}

// =============================================================================
// CVPixelBufferWrapper 实现 (iOS/macOS)
// =============================================================================

#ifdef __APPLE__

CVPixelBufferWrapper::CVPixelBufferWrapper(void* pixelBuffer)
    : mPixelBuffer(pixelBuffer) {
    if (mPixelBuffer) {
        CVPixelBufferRetain((CVPixelBufferRef)mPixelBuffer);
        ExtractImageDesc();
    }
}

CVPixelBufferWrapper::CVPixelBufferWrapper(const ImageDataDesc& imageDesc)
    : mImageDesc(imageDesc) {
    CreatePixelBuffer();
}

CVPixelBufferWrapper::~CVPixelBufferWrapper() {
    if (mIsLocked) {
        Unlock();
    }
    if (mPixelBuffer) {
        CVPixelBufferRelease((CVPixelBufferRef)mPixelBuffer);
        mPixelBuffer = nullptr;
    }
}

void CVPixelBufferWrapper::ExtractImageDesc() {
    if (!mPixelBuffer) {
        return;
    }

    CVPixelBufferRef pb = (CVPixelBufferRef)mPixelBuffer;
    mImageDesc.width = static_cast<uint32_t>(CVPixelBufferGetWidth(pb));
    mImageDesc.height = static_cast<uint32_t>(CVPixelBufferGetHeight(pb));

    // 根据 CVPixelBuffer 格式映射到 ImageFormat
    OSType pixelFormat = CVPixelBufferGetPixelFormatType(pb);
    switch (pixelFormat) {
        case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange:
        case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:
            mImageDesc.format = ImageFormat::NV12;
            mImageDesc.colorSpace = ColorSpace::BT709;
            mImageDesc.range = (pixelFormat == kCVPixelFormatType_420YpCbCr8BiPlanarFullRange)
                                   ? ColorRange::Full : ColorRange::Video;
            break;
        case kCVPixelFormatType_32BGRA:
            mImageDesc.format = ImageFormat::BGRA8;
            mImageDesc.colorSpace = ColorSpace::BT709;
            mImageDesc.range = ColorRange::Full;
            break;
        case kCVPixelFormatType_32RGBA:
            mImageDesc.format = ImageFormat::RGBA8;
            mImageDesc.colorSpace = ColorSpace::BT709;
            mImageDesc.range = ColorRange::Full;
            break;
        default:
            mImageDesc.format = ImageFormat::Unknown;
            break;
    }

    // 平面信息（锁定后才能访问）
    size_t planeCount = CVPixelBufferGetPlaneCount(pb);
    if (planeCount == 0) {
        planeCount = 1; // 非平面格式
    }
    mImageDesc.planes.resize(planeCount);
}

void CVPixelBufferWrapper::CreatePixelBuffer() {
    // 将 ImageFormat 映射到 CVPixelBuffer 格式
    OSType pixelFormat = kCVPixelFormatType_32BGRA; // 默认
    switch (mImageDesc.format) {
        case ImageFormat::NV12:
            pixelFormat = (mImageDesc.range == ColorRange::Full)
                         ? kCVPixelFormatType_420YpCbCr8BiPlanarFullRange
                         : kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange;
            break;
        case ImageFormat::BGRA8:
            pixelFormat = kCVPixelFormatType_32BGRA;
            break;
        case ImageFormat::RGBA8:
            pixelFormat = kCVPixelFormatType_32RGBA;
            break;
        default:
            // 不支持的格式
            return;
    }

    CVPixelBufferRef pb = nullptr;
    CVReturn status = CVPixelBufferCreate(
        kCFAllocatorDefault,
        mImageDesc.width,
        mImageDesc.height,
        pixelFormat,
        nullptr, // attributes
        &pb
    );

    if (status == kCVReturnSuccess && pb) {
        mPixelBuffer = pb;
        ExtractImageDesc();
    }
}

bool CVPixelBufferWrapper::Lock(bool readOnly) {
    if (!mPixelBuffer || mIsLocked) {
        return false;
    }

    CVPixelBufferRef pb = (CVPixelBufferRef)mPixelBuffer;
    CVOptionFlags lockFlags = readOnly ? kCVPixelBufferLock_ReadOnly : 0;
    CVReturn status = CVPixelBufferLockBaseAddress(pb, lockFlags);
    
    if (status == kCVReturnSuccess) {
        mIsLocked = true;

        // 更新平面数据指针
        size_t planeCount = CVPixelBufferGetPlaneCount(pb);
        if (planeCount == 0) {
            // 非平面格式
            mImageDesc.planes.resize(1);
            mImageDesc.planes[0].data = CVPixelBufferGetBaseAddress(pb);
            mImageDesc.planes[0].stride = static_cast<uint32_t>(CVPixelBufferGetBytesPerRow(pb));
        } else {
            // 多平面格式
            mImageDesc.planes.resize(planeCount);
            for (size_t i = 0; i < planeCount; ++i) {
                mImageDesc.planes[i].data = CVPixelBufferGetBaseAddressOfPlane(pb, i);
                mImageDesc.planes[i].stride = static_cast<uint32_t>(CVPixelBufferGetBytesPerRowOfPlane(pb, i));
            }
        }
        return true;
    }
    
    return false;
}

void CVPixelBufferWrapper::Unlock() {
    if (!mPixelBuffer || !mIsLocked) {
        return;
    }

    CVPixelBufferRef pb = (CVPixelBufferRef)mPixelBuffer;
    CVPixelBufferUnlockBaseAddress(pb, 0);
    mIsLocked = false;

    // 清除平面数据指针（不再有效）
    for (auto& plane : mImageDesc.planes) {
        plane.data = nullptr;
    }
}

#endif // __APPLE__

// =============================================================================
// HardwareBufferWrapper 实现 (Android)
// =============================================================================

#ifdef __ANDROID__

HardwareBufferWrapper::HardwareBufferWrapper(void* hardwareBuffer)
    : mHardwareBuffer(hardwareBuffer) {
    if (mHardwareBuffer) {
        AHardwareBuffer_acquire((AHardwareBuffer*)mHardwareBuffer);
        ExtractImageDesc();
    }
}

HardwareBufferWrapper::HardwareBufferWrapper(const ImageDataDesc& imageDesc)
    : mImageDesc(imageDesc) {
    CreateHardwareBuffer();
}

HardwareBufferWrapper::~HardwareBufferWrapper() {
    if (mIsLocked) {
        Unlock();
    }
    if (mHardwareBuffer) {
        AHardwareBuffer_release((AHardwareBuffer*)mHardwareBuffer);
        mHardwareBuffer = nullptr;
    }
}

void HardwareBufferWrapper::ExtractImageDesc() {
    if (!mHardwareBuffer) {
        return;
    }

    AHardwareBuffer* ahb = (AHardwareBuffer*)mHardwareBuffer;
    AHardwareBuffer_Desc desc;
    AHardwareBuffer_describe(ahb, &desc);

    mImageDesc.width = desc.width;
    mImageDesc.height = desc.height;

    // 映射 AHardwareBuffer 格式到 ImageFormat
    switch (desc.format) {
        case AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM:
            mImageDesc.format = ImageFormat::RGBA8;
            mImageDesc.colorSpace = ColorSpace::BT709;
            mImageDesc.range = ColorRange::Full;
            break;
        case AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420:
            mImageDesc.format = ImageFormat::YUV420P;
            mImageDesc.colorSpace = ColorSpace::BT709;
            mImageDesc.range = ColorRange::Video;
            break;
        default:
            mImageDesc.format = ImageFormat::Unknown;
            break;
    }
}

void HardwareBufferWrapper::CreateHardwareBuffer() {
    // 映射 ImageFormat 到 AHardwareBuffer 格式
    uint32_t format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM; // 默认
    switch (mImageDesc.format) {
        case ImageFormat::RGBA8:
            format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
            break;
        case ImageFormat::YUV420P:
            format = AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420;
            break;
        default:
            // 不支持的格式
            return;
    }

    AHardwareBuffer_Desc desc = {};
    desc.width = mImageDesc.width;
    desc.height = mImageDesc.height;
    desc.layers = 1;
    desc.format = format;
    desc.usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN |
                 AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN |
                 AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE;

    AHardwareBuffer* ahb = nullptr;
    int result = AHardwareBuffer_allocate(&desc, &ahb);
    
    if (result == 0 && ahb) {
        mHardwareBuffer = ahb;
        ExtractImageDesc();
    }
}

bool HardwareBufferWrapper::Lock(bool readOnly) {
    if (!mHardwareBuffer || mIsLocked) {
        return false;
    }

    AHardwareBuffer* ahb = (AHardwareBuffer*)mHardwareBuffer;
    
    uint64_t usage = readOnly ? AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN
                              : AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN;
    
    void* data = nullptr;
    int result = AHardwareBuffer_lock(ahb, usage, -1, nullptr, &data);
    
    if (result == 0 && data) {
        mIsLocked = true;

        // 简化处理：将整个 buffer 作为单平面
        AHardwareBuffer_Desc desc;
        AHardwareBuffer_describe(ahb, &desc);
        
        mImageDesc.planes.resize(1);
        mImageDesc.planes[0].data = data;
        mImageDesc.planes[0].stride = desc.stride;
        
        return true;
    }
    
    return false;
}

void HardwareBufferWrapper::Unlock() {
    if (!mHardwareBuffer || !mIsLocked) {
        return;
    }

    AHardwareBuffer* ahb = (AHardwareBuffer*)mHardwareBuffer;
    AHardwareBuffer_unlock(ahb, nullptr);
    mIsLocked = false;

    // 清除平面数据指针
    for (auto& plane : mImageDesc.planes) {
        plane.data = nullptr;
    }
}

#endif // __ANDROID__

} // namespace utils
} // namespace lrengine
