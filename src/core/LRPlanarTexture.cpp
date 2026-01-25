/**
 * @file LRPlanarTexture.cpp
 * @brief LREngine 多平面纹理实现
 */

#include "lrengine/core/LRPlanarTexture.h"
#include "lrengine/core/LRRenderContext.h"
#include "lrengine/core/LRTexture.h"

namespace lrengine {
namespace render {

LRPlanarTexture::LRPlanarTexture()
    : LRResource(ResourceType::Texture) {
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
    if (mPlanes.empty() || !mPlanes[0]) {
        ResourceHandle h;
        h.ptr = nullptr;
        return h;
    }
    return mPlanes[0]->GetNativeHandle();
}

} // namespace render
} // namespace lrengine
