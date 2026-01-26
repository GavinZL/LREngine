/**
 * @file LRPlanarTexture.cpp
 * @brief LREngine 多平面纹理实现
 */

#include "lrengine/core/LRPlanarTexture.h"
#include "lrengine/core/LRRenderContext.h"
#include "lrengine/core/LRTexture.h"
#include "lrengine/utils/ImageBufferPool.h"
#include "platform/interface/ITextureImpl.h"

namespace lrengine {
namespace render {

LRPlanarTexture::LRPlanarTexture()
    : LRResource(ResourceType::Texture)
    , mImageFormat(ImageFormat::Unknown) {
}

LRPlanarTexture::~LRPlanarTexture() {
    releasePlanes();
}

bool LRPlanarTexture::Initialize(LRRenderContext* context, const PlanarTextureDescriptor& desc) {
    if (!context || desc.width == 0 || desc.height == 0) {
        return false;
    }
    
    mContext = context;
    mWidth = desc.width;
    mHeight = desc.height;
    mFormat = desc.format;
    
    switch (mFormat) {
        case PlanarFormat::YUV420P: mImageFormat = ImageFormat::YUV420P; break;
        case PlanarFormat::NV12:    mImageFormat = ImageFormat::NV12;    break;
        case PlanarFormat::NV21:    mImageFormat = ImageFormat::NV21;    break;
        case PlanarFormat::RGBA:    mImageFormat = ImageFormat::RGBA8;   break;
        default:                    mImageFormat = ImageFormat::Unknown; break;
    }
    
    if (desc.debugName) {
        SetDebugName(desc.debugName);
    }
    
    if (!createPlanes(desc)) {
        return false;
    }
    
    mIsValid = true;
    return true;
}

bool LRPlanarTexture::createPlanes(const PlanarTextureDescriptor& desc) {
    // 释放旧的平面
    releasePlanes();
    
    switch (desc.format) {
        case PlanarFormat::YUV420P: {
            // Y 平面: 全尺寸 R8
            TextureDescriptor yDesc;
            yDesc.width = desc.width;
            yDesc.height = desc.height;
            yDesc.depth = 1;
            yDesc.type = TextureType::Texture2D;
            yDesc.format = PixelFormat::R8;
            yDesc.mipLevels = 1;
            yDesc.sampler = desc.sampler;
            yDesc.debugName = "PlanarTexture_Y";
            
            // U/V 平面: 1/4 尺寸 R8
            TextureDescriptor uvDesc;
            uvDesc.width = desc.width / 2;
            uvDesc.height = desc.height / 2;
            uvDesc.depth = 1;
            uvDesc.type = TextureType::Texture2D;
            uvDesc.format = PixelFormat::R8;
            uvDesc.mipLevels = 1;
            uvDesc.sampler = desc.sampler;
            
            auto* yTex = mContext->CreateTexture(yDesc);
            if (!yTex) return false;
            
            uvDesc.debugName = "PlanarTexture_U";
            auto* uTex = mContext->CreateTexture(uvDesc);
            if (!uTex) {
                delete yTex;
                return false;
            }
            
            uvDesc.debugName = "PlanarTexture_V";
            auto* vTex = mContext->CreateTexture(uvDesc);
            if (!vTex) {
                delete yTex;
                delete uTex;
                return false;
            }
            
            mPlanes = {yTex, uTex, vTex};
            break;
        }
        
        case PlanarFormat::NV12:
        case PlanarFormat::NV21: {
            // Y 平面: 全尺寸 R8
            TextureDescriptor yDesc;
            yDesc.width = desc.width;
            yDesc.height = desc.height;
            yDesc.depth = 1;
            yDesc.type = TextureType::Texture2D;
            yDesc.format = PixelFormat::R8;
            yDesc.mipLevels = 1;
            yDesc.sampler = desc.sampler;
            yDesc.debugName = "PlanarTexture_Y";
            
            // UV 平面: 1/2 尺寸 RG8
            TextureDescriptor uvDesc;
            uvDesc.width = desc.width / 2;
            uvDesc.height = desc.height / 2;
            uvDesc.depth = 1;
            uvDesc.type = TextureType::Texture2D;
            uvDesc.format = PixelFormat::RG8;
            uvDesc.mipLevels = 1;
            uvDesc.sampler = desc.sampler;
            uvDesc.debugName = (desc.format == PlanarFormat::NV12) ? "PlanarTexture_UV" : "PlanarTexture_VU";
            
            auto* yTex = mContext->CreateTexture(yDesc);
            if (!yTex) return false;
            
            auto* uvTex = mContext->CreateTexture(uvDesc);
            if (!uvTex) {
                delete yTex;
                return false;
            }
            
            mPlanes = {yTex, uvTex};
            break;
        }
        
        case PlanarFormat::RGBA: {
            // 兼容模式: 单平面 RGBA8
            TextureDescriptor rgbaDesc;
            rgbaDesc.width = desc.width;
            rgbaDesc.height = desc.height;
            rgbaDesc.depth = 1;
            rgbaDesc.type = TextureType::Texture2D;
            rgbaDesc.format = PixelFormat::RGBA8;
            rgbaDesc.mipLevels = 1;
            rgbaDesc.sampler = desc.sampler;
            rgbaDesc.debugName = "PlanarTexture_RGBA";
            
            auto* rgbaTex = mContext->CreateTexture(rgbaDesc);
            if (!rgbaTex) return false;
            
            mPlanes = {rgbaTex};
            break;
        }
        
        default:
            return false;
    }
    
    return !mPlanes.empty();
}

void LRPlanarTexture::releasePlanes() {
    for (auto* plane : mPlanes) {
        if (plane) {
            delete plane;
        }
    }
    mPlanes.clear();
}

LRTexture* LRPlanarTexture::GetPlaneTexture(uint32_t planeIndex) const {
    if (planeIndex >= mPlanes.size()) {
        return nullptr;
    }
    return mPlanes[planeIndex];
}

void LRPlanarTexture::UpdatePlaneData(uint32_t planeIndex, const void* data, uint32_t /*stride*/) {
    if (planeIndex >= mPlanes.size() || !data) {
        return;
    }
    
    // 更新整个纹理数据
    mPlanes[planeIndex]->UpdateData(data, nullptr);
}

void LRPlanarTexture::UpdateAllPlanes(const std::vector<const void*>& planeData,
                                      const std::vector<uint32_t>& strides) {
    size_t count = std::min(mPlanes.size(), planeData.size());
    for (size_t i = 0; i < count; ++i) {
        if (planeData[i]) {
            uint32_t stride = (i < strides.size()) ? strides[i] : 0;
            UpdatePlaneData(static_cast<uint32_t>(i), planeData[i], stride);
        }
    }
}

bool LRPlanarTexture::UpdateFromImage(const ImageDataDesc& imageData,
                                     const UpdateFromImageOptions& options) {
    if (!mIsValid) {
        return false;
    }
    if (imageData.width == 0 || imageData.height == 0) {
        return false;
    }
    if (imageData.width != mWidth || imageData.height != mHeight) {
        return false;
    }
    if (imageData.planes.empty()) {
        return false;
    }

    // TODO: 处理 flipVertically 和 targetRegion
    (void)options;

    switch (imageData.format) {
        case ImageFormat::NV12:
        case ImageFormat::NV21: {
            if (imageData.planes.size() < 2 || mPlanes.size() < 2) {
                return false;
            }
            UpdatePlaneData(0, imageData.planes[0].data, imageData.planes[0].stride);
            UpdatePlaneData(1, imageData.planes[1].data, imageData.planes[1].stride);
            break;
        }
        case ImageFormat::YUV420P: {
            if (imageData.planes.size() < 3 || mPlanes.size() < 3) {
                return false;
            }
            UpdatePlaneData(0, imageData.planes[0].data, imageData.planes[0].stride);
            UpdatePlaneData(1, imageData.planes[1].data, imageData.planes[1].stride);
            UpdatePlaneData(2, imageData.planes[2].data, imageData.planes[2].stride);
            break;
        }
        case ImageFormat::RGBA8:
        case ImageFormat::BGRA8:
        case ImageFormat::RGB8:
        case ImageFormat::GRAY8: {
            if (mPlanes.empty()) {
                return false;
            }
            UpdatePlaneData(0, imageData.planes[0].data, imageData.planes[0].stride);
            break;
        }
        default:
            return false;
    }

    // 如果需要生成 mipmap
    if (options.generateMipmaps) {
        for (auto* plane : mPlanes) {
            if (plane) {
                plane->GenerateMipmaps();
            }
        }
    }

    mImageFormat = imageData.format;
    return true;
}

void LRPlanarTexture::BindAll(uint32_t baseSlot) {
    mBoundBaseSlot = baseSlot;
    for (size_t i = 0; i < mPlanes.size(); ++i) {
        if (mPlanes[i]) {
            mPlanes[i]->Bind(baseSlot + static_cast<uint32_t>(i));
        }
    }
}

void LRPlanarTexture::UnbindAll() {
    for (auto* plane : mPlanes) {
        if (plane) {
            plane->Unbind();
        }
    }
}

void LRPlanarTexture::BindPlane(uint32_t planeIndex, uint32_t slot) {
    if (planeIndex < mPlanes.size() && mPlanes[planeIndex]) {
        mPlanes[planeIndex]->Bind(slot);
    }
}

void LRPlanarTexture::UnbindPlane(uint32_t planeIndex, uint32_t /*slot*/) {
    if (planeIndex < mPlanes.size() && mPlanes[planeIndex]) {
        mPlanes[planeIndex]->Unbind();
    }
}

void LRPlanarTexture::GetPlaneSize(uint32_t planeIndex, uint32_t& width, uint32_t& height) const {
    if (planeIndex >= mPlanes.size() || !mPlanes[planeIndex]) {
        width = 0;
        height = 0;
        return;
    }
    
    width = mPlanes[planeIndex]->GetWidth();
    height = mPlanes[planeIndex]->GetHeight();
}

PixelFormat LRPlanarTexture::GetPlaneFormat(uint32_t planeIndex) const {
    if (planeIndex >= mPlanes.size() || !mPlanes[planeIndex]) {
        return PixelFormat::RGBA8;
    }
    return mPlanes[planeIndex]->GetFormat();
}

ResourceHandle LRPlanarTexture::GetNativeHandle() const {
    return GetNativeHandle(0);
}

ResourceHandle LRPlanarTexture::GetNativeHandle(uint32_t planeIndex) const {
    if (planeIndex >= mPlanes.size() || !mPlanes[planeIndex]) {
        ResourceHandle h;
        h.ptr = nullptr;
        return h;
    }
    return mPlanes[planeIndex]->GetNativeHandle();
}

bool LRPlanarTexture::Readback(ReadbackResult& outResult, const ReadbackOptions& options) {
    outResult.success = false;
    
    if (!mIsValid || mPlanes.empty()) {
        return false;
    }

    // 确定目标格式（默认使用当前纹理格式）
    ImageFormat targetFormat = options.targetFormat;
    if (targetFormat == ImageFormat::Unknown) {
        targetFormat = mImageFormat;
    }

    // 准备图像描述
    ImageDataDesc imageDesc;
    imageDesc.width = mWidth;
    imageDesc.height = mHeight;
    imageDesc.format = targetFormat;
    imageDesc.colorSpace = (options.targetColorSpace != ColorSpace::Unknown) 
                          ? options.targetColorSpace : ColorSpace::BT709;
    imageDesc.range = (options.targetColorRange != ColorRange::Unknown)
                     ? options.targetColorRange : ColorRange::Video;

    // 使用对象池分配缓冲区
    // TODO: 这里应该使用线程安全的全局对象池
    // 暂时直接创建缓冲区
    std::unique_ptr<utils::ImageBuffer> buffer;
    
    #ifdef __APPLE__
    // macOS/iOS: 优先使用 CVPixelBuffer
    if (options.asyncReadback) {
        // 异步回读使用 CVPixelBuffer 可以实现零拷贝
        buffer = std::make_unique<utils::CVPixelBufferWrapper>(imageDesc);
    } else {
        // 同步回读使用主机内存
        buffer = std::make_unique<utils::HostMemoryBuffer>(imageDesc, true);
    }
    #else
    // 其他平台：使用主机内存
    buffer = std::make_unique<utils::HostMemoryBuffer>(imageDesc, true);
    #endif

    if (!buffer) {
        return false;
    }

    // 对于多平面纹理，需要分别回读每个平面
    // 暂时只处理单平面格式（RGBA、BGRA 等）
    if (mPlanes.size() == 1) {
        // 单平面：直接回读
        auto* plane = mPlanes[0];
        if (plane && plane->GetImpl()) {
            if (plane->GetImpl()->ReadbackTo(buffer.get(), 0)) {
                outResult.success = true;
                outResult.imageData = buffer->GetImageDesc();
                outResult.nativeBuffer = buffer->GetNativeBuffer();
                
                // 将缓冲区所有权转移给用户
                // 注意：这里需要用户自己管理缓冲区生命周期
                // 或者后续实现使用 shared_ptr
                buffer.release();  // 释放所有权
                return true;
            }
        }
    } else {
        // 多平面（YUV 等）：TODO 后续实现
        // 需要为每个平面创建缓冲区并分别回读
    }

    return false;
}

} // namespace render
} // namespace lrengine
