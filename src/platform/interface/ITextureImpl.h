/**
 * @file ITextureImpl.h
 * @brief 纹理平台实现接口
 */

#pragma once

#include "lrengine/core/LRTypes.h"

namespace lrengine {
namespace render {

/**
 * @brief 纹理实现接口
 */
class ITextureImpl {
public:
    virtual ~ITextureImpl() = default;
    
    /**
     * @brief 创建纹理
     * @param desc 纹理描述符
     * @return 成功返回true
     */
    virtual bool Create(const TextureDescriptor& desc) = 0;
    
    /**
     * @brief 销毁纹理
     */
    virtual void Destroy() = 0;
    
    /**
     * @brief 更新纹理数据
     * @param data 像素数据
     * @param mipLevel mipmap级别
     * @param region 更新区域（nullptr表示整个纹理）
     */
    virtual void UpdateData(const void* data, uint32_t mipLevel = 0, const TextureRegion* region = nullptr) = 0;
    
    /**
     * @brief 生成Mipmap
     */
    virtual void GenerateMipmaps() = 0;
    
    /**
     * @brief 绑定到纹理单元
     * @param slot 纹理单元索引
     */
    virtual void Bind(uint32_t slot) = 0;
    
    /**
     * @brief 解绑纹理
     * @param slot 纹理单元索引
     */
    virtual void Unbind(uint32_t slot) = 0;
    
    /**
     * @brief 获取原生句柄
     */
    virtual ResourceHandle GetNativeHandle() const = 0;
    
    /**
     * @brief 获取宽度
     */
    virtual uint32_t GetWidth() const = 0;
    
    /**
     * @brief 获取高度
     */
    virtual uint32_t GetHeight() const = 0;
    
    /**
     * @brief 获取深度
     */
    virtual uint32_t GetDepth() const = 0;
    
    /**
     * @brief 获取纹理类型
     */
    virtual TextureType GetType() const = 0;
    
    /**
     * @brief 获取像素格式
     */
    virtual PixelFormat GetFormat() const = 0;
    
    /**
     * @brief 获取Mipmap层数
     */
    virtual uint32_t GetMipLevels() const = 0;
};

} // namespace render
} // namespace lrengine
