/**
 * @file LRTexture.h
 * @brief LREngine纹理对象
 */

#pragma once

#include "LRResource.h"
#include "LRTypes.h"

namespace lrengine {
namespace render {

class ITextureImpl;

/**
 * @brief 纹理类
 */
class LR_API LRTexture : public LRResource {
public:
    LR_NONCOPYABLE(LRTexture);
    
    virtual ~LRTexture();
    
    /**
     * @brief 更新纹理数据
     * @param data 像素数据
     * @param region 更新区域（nullptr表示整个纹理）
     */
    void UpdateData(const void* data, const TextureRegion* region = nullptr);
    
    /**
     * @brief 生成Mipmap
     */
    void GenerateMipmaps();
    
    /**
     * @brief 绑定到纹理单元
     * @param slot 纹理单元索引
     */
    void Bind(uint32_t slot = 0);
    
    /**
     * @brief 解绑纹理
     */
    void Unbind();
    
    /**
     * @brief 获取宽度
     */
    uint32_t GetWidth() const { return mWidth; }
    
    /**
     * @brief 获取高度
     */
    uint32_t GetHeight() const { return mHeight; }
    
    /**
     * @brief 获取深度（3D纹理或数组层数）
     */
    uint32_t GetDepth() const { return mDepth; }
    
    /**
     * @brief 获取纹理类型
     */
    TextureType GetTextureType() const { return mTextureType; }
    
    /**
     * @brief 获取像素格式
     */
    PixelFormat GetFormat() const { return mFormat; }
    
    /**
     * @brief 获取Mipmap层数
     */
    uint32_t GetMipLevels() const { return mMipLevels; }
    
    /**
     * @brief 获取采样数
     */
    uint32_t GetSamples() const { return mSamples; }
    
    /**
     * @brief 获取原生句柄
     */
    ResourceHandle GetNativeHandle() const override;
    
    /**
     * @brief 获取后端实现指针
     */
    ITextureImpl* GetImpl() const { return mImpl; }
    
protected:
    friend class LRRenderContext;
    
    LRTexture();
    bool Initialize(ITextureImpl* impl, const TextureDescriptor& desc);
    
protected:
    ITextureImpl* mImpl = nullptr;
    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    uint32_t mDepth = 1;
    TextureType mTextureType = TextureType::Texture2D;
    PixelFormat mFormat = PixelFormat::RGBA8;
    uint32_t mMipLevels = 1;
    uint32_t mSamples = 1;
    uint32_t mBoundSlot = 0;
};

} // namespace render
} // namespace lrengine
