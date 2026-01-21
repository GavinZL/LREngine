/**
 * @file IFrameBufferImpl.h
 * @brief 帧缓冲平台实现接口
 */

#pragma once

#include "lrengine/core/LRTypes.h"

namespace lrengine {
namespace render {

class ITextureImpl;

/**
 * @brief 帧缓冲实现接口
 */
class IFrameBufferImpl {
public:
    virtual ~IFrameBufferImpl() = default;

    /**
     * @brief 创建帧缓冲
     * @param desc 帧缓冲描述符
     * @return 成功返回true
     */
    virtual bool Create(const FrameBufferDescriptor& desc) = 0;

    /**
     * @brief 销毁帧缓冲
     */
    virtual void Destroy() = 0;

    /**
     * @brief 附加颜色纹理
     * @param texture 纹理实现
     * @param index 附件索引
     * @param mipLevel mipmap级别
     * @return 成功返回true
     */
    virtual bool AttachColorTexture(ITextureImpl* texture, uint32_t index, uint32_t mipLevel = 0) = 0;

    /**
     * @brief 附加深度纹理
     * @param texture 纹理实现
     * @param mipLevel mipmap级别
     * @return 成功返回true
     */
    virtual bool AttachDepthTexture(ITextureImpl* texture, uint32_t mipLevel = 0) = 0;

    /**
     * @brief 附加模板纹理
     * @param texture 纹理实现
     * @param mipLevel mipmap级别
     * @return 成功返回true
     */
    virtual bool AttachStencilTexture(ITextureImpl* texture, uint32_t mipLevel = 0) = 0;

    /**
     * @brief 附加深度模板纹理
     * @param texture 纹理实现
     * @param mipLevel mipmap级别
     * @return 成功返回true
     */
    virtual bool AttachDepthStencilTexture(ITextureImpl* texture, uint32_t mipLevel = 0) = 0;

    /**
     * @brief 检查帧缓冲是否完整
     */
    virtual bool IsComplete() const = 0;

    /**
     * @brief 绑定帧缓冲
     */
    virtual void Bind() = 0;

    /**
     * @brief 解绑帧缓冲
     */
    virtual void Unbind() = 0;

    /**
     * @brief 清除帧缓冲
     * @param flags 清除标志
     * @param color 清除颜色
     * @param depth 清除深度
     * @param stencil 清除模板
     */
    virtual void Clear(uint32_t flags, const float* color, float depth, uint8_t stencil) = 0;

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
     * @brief 获取颜色附件数量
     */
    virtual uint32_t GetColorAttachmentCount() const = 0;
};

} // namespace render
} // namespace lrengine
