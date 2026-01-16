/**
 * @file LRFrameBuffer.h
 * @brief LREngine帧缓冲对象
 */

#pragma once

#include "LRResource.h"
#include "LRTypes.h"

#include <vector>

namespace lrengine {
namespace render {

class IFrameBufferImpl;
class LRTexture;

/**
 * @brief 帧缓冲类
 * 
 * 支持离屏渲染和多重渲染目标(MRT)
 */
class LR_API LRFrameBuffer : public LRResource {
public:
    LR_NONCOPYABLE(LRFrameBuffer);
    
    virtual ~LRFrameBuffer();
    
    /**
     * @brief 附加颜色纹理
     * @param texture 纹理对象
     * @param index 附件索引（最多8个）
     */
    void AttachColorTexture(LRTexture* texture, uint32_t index = 0);
    
    /**
     * @brief 附加深度纹理
     * @param texture 深度纹理对象
     */
    void AttachDepthTexture(LRTexture* texture);
    
    /**
     * @brief 附加模板纹理
     * @param texture 模板纹理对象
     */
    void AttachStencilTexture(LRTexture* texture);
    
    /**
     * @brief 检查帧缓冲是否完整
     */
    bool IsComplete() const;
    
    /**
     * @brief 绑定帧缓冲
     */
    void Bind();
    
    /**
     * @brief 解绑帧缓冲（恢复默认）
     */
    void Unbind();
    
    /**
     * @brief 清除缓冲区
     * @param flags 清除标志
     * @param color 清除颜色
     * @param depth 清除深度
     * @param stencil 清除模板
     */
    void Clear(uint8_t flags, const float* color, float depth = 1.0f, uint8_t stencil = 0);
    
    /**
     * @brief 获取颜色附件
     * @param index 附件索引
     */
    LRTexture* GetColorTexture(uint32_t index = 0) const;
    
    /**
     * @brief 获取深度附件
     */
    LRTexture* GetDepthTexture() const { return mDepthTexture; }
    
    /**
     * @brief 获取宽度
     */
    uint32_t GetWidth() const { return mWidth; }
    
    /**
     * @brief 获取高度
     */
    uint32_t GetHeight() const { return mHeight; }
    
    /**
     * @brief 获取颜色附件数量
     */
    uint32_t GetColorAttachmentCount() const { return static_cast<uint32_t>(mColorTextures.size()); }
    
    /**
     * @brief 获取原生句柄
     */
    ResourceHandle GetNativeHandle() const override;
    
protected:
    friend class LRRenderContext;
    
    LRFrameBuffer();
    bool Initialize(IFrameBufferImpl* impl, const FrameBufferDescriptor& desc);
    
protected:
    IFrameBufferImpl* mImpl = nullptr;
    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    std::vector<LRTexture*> mColorTextures;
    LRTexture* mDepthTexture = nullptr;
    LRTexture* mStencilTexture = nullptr;
};

} // namespace render
} // namespace lrengine
