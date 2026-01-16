/**
 * @file FrameBufferMTL.h
 * @brief Metal帧缓冲实现
 */

#pragma once

#include "platform/interface/IFrameBufferImpl.h"
#include <vector>

#ifdef LRENGINE_ENABLE_METAL

#import <Metal/Metal.h>

namespace lrengine {
namespace render {
namespace mtl {

class TextureMTL;

/**
 * @brief Metal帧缓冲实现
 * 
 * Metal没有帧缓冲对象的概念，而是使用渲染通道描述符
 * 这个类管理渲染目标纹理并创建MTLRenderPassDescriptor
 */
class FrameBufferMTL : public IFrameBufferImpl {
public:
    FrameBufferMTL(id<MTLDevice> device);
    ~FrameBufferMTL() override;

    // IFrameBufferImpl接口
    bool Create(const FrameBufferDescriptor& desc) override;
    void Destroy() override;
    bool AttachColorTexture(ITextureImpl* texture, uint32_t index, uint32_t mipLevel = 0) override;
    bool AttachDepthTexture(ITextureImpl* texture, uint32_t mipLevel = 0) override;
    bool AttachStencilTexture(ITextureImpl* texture, uint32_t mipLevel = 0) override;
    bool AttachDepthStencilTexture(ITextureImpl* texture, uint32_t mipLevel = 0) override;
    bool IsComplete() const override;
    void Bind() override;
    void Unbind() override;
    void Clear(uint32_t flags, const float* color, float depth, uint8_t stencil) override;
    ResourceHandle GetNativeHandle() const override;
    uint32_t GetWidth() const override;
    uint32_t GetHeight() const override;
    uint32_t GetColorAttachmentCount() const override;

    // Metal特有方法
    MTLRenderPassDescriptor* GetRenderPassDescriptor() const { return m_renderPassDescriptor; }
    id<MTLTexture> GetColorTexture(uint32_t index) const;
    id<MTLTexture> GetDepthTexture() const { return m_depthTexture; }
    id<MTLTexture> GetStencilTexture() const { return m_stencilTexture; }

private:
    void UpdateRenderPassDescriptor();

    id<MTLDevice> m_device;
    MTLRenderPassDescriptor* m_renderPassDescriptor;
    
    std::vector<id<MTLTexture>> m_colorTextures;
    std::vector<uint32_t> m_colorMipLevels;
    id<MTLTexture> m_depthTexture;
    id<MTLTexture> m_stencilTexture;
    uint32_t m_depthMipLevel;
    uint32_t m_stencilMipLevel;
    
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_samples;
    
    // 清除值
    float m_clearColor[4];
    float m_clearDepth;
    uint8_t m_clearStencil;
    
    FrameBufferDescriptor m_descriptor;
};

} // namespace mtl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
