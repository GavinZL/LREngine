/**
 * @file FrameBufferMTL.mm
 * @brief Metal帧缓冲实现
 */

#include "FrameBufferMTL.h"
#include "TextureMTL.h"

#ifdef LRENGINE_ENABLE_METAL

#include "TypeConverterMTL.h"
#include "lrengine/core/LRError.h"

namespace lrengine {
namespace render {
namespace mtl {

FrameBufferMTL::FrameBufferMTL(id<MTLDevice> device)
    : m_device(device)
    , m_renderPassDescriptor(nil)
    , m_depthTexture(nil)
    , m_stencilTexture(nil)
    , m_depthMipLevel(0)
    , m_stencilMipLevel(0)
    , m_width(0)
    , m_height(0)
    , m_samples(1)
    , m_clearDepth(1.0f)
    , m_clearStencil(0)
{
    m_clearColor[0] = 0.0f;
    m_clearColor[1] = 0.0f;
    m_clearColor[2] = 0.0f;
    m_clearColor[3] = 1.0f;
}

FrameBufferMTL::~FrameBufferMTL() {
    Destroy();
}

bool FrameBufferMTL::Create(const FrameBufferDescriptor& desc) {
    m_descriptor = desc;
    m_width = desc.width;
    m_height = desc.height;
    m_samples = desc.samples;

    // 创建颜色附件纹理
    for (const auto& colorAttachment : desc.colorAttachments) {
        MTLTextureDescriptor* texDesc = [[MTLTextureDescriptor alloc] init];
        texDesc.textureType = (m_samples > 1) ? MTLTextureType2DMultisample : MTLTextureType2D;
        texDesc.pixelFormat = ToMTLPixelFormat(colorAttachment.format);
        texDesc.width = m_width;
        texDesc.height = m_height;
        texDesc.sampleCount = m_samples;
        texDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        texDesc.storageMode = MTLStorageModePrivate;

        id<MTLTexture> colorTexture = [m_device newTextureWithDescriptor:texDesc];
        if (!colorTexture) {
            LR_SET_ERROR(ErrorCode::ResourceCreationFailed, "Failed to create color attachment texture");
            return false;
        }

        m_colorTextures.push_back(colorTexture);
        m_colorMipLevels.push_back(0);
        
        // 保存清除颜色
        memcpy(m_clearColor, colorAttachment.clearColor, sizeof(m_clearColor));
    }

    // 创建深度/模板纹理
    if (desc.hasDepthStencil) {
        MTLTextureDescriptor* depthDesc = [[MTLTextureDescriptor alloc] init];
        depthDesc.textureType = (m_samples > 1) ? MTLTextureType2DMultisample : MTLTextureType2D;
        depthDesc.pixelFormat = ToMTLPixelFormat(desc.depthStencilAttachment.format);
        depthDesc.width = m_width;
        depthDesc.height = m_height;
        depthDesc.sampleCount = m_samples;
        depthDesc.usage = MTLTextureUsageRenderTarget;
        depthDesc.storageMode = MTLStorageModePrivate;

        m_depthTexture = [m_device newTextureWithDescriptor:depthDesc];
        if (!m_depthTexture) {
            LR_SET_ERROR(ErrorCode::ResourceCreationFailed, "Failed to create depth attachment texture");
            return false;
        }

        m_clearDepth = desc.depthStencilAttachment.clearDepth;
        m_clearStencil = desc.depthStencilAttachment.clearStencil;
    }

    // 创建渲染通道描述符
    m_renderPassDescriptor = [[MTLRenderPassDescriptor alloc] init];
    UpdateRenderPassDescriptor();

    return true;
}

void FrameBufferMTL::Destroy() {
    m_colorTextures.clear();
    m_colorMipLevels.clear();
    m_depthTexture = nil;
    m_stencilTexture = nil;
    m_renderPassDescriptor = nil;
    m_width = 0;
    m_height = 0;
}

bool FrameBufferMTL::AttachColorTexture(ITextureImpl* texture, uint32_t index, uint32_t mipLevel) {
    TextureMTL* mtlTexture = static_cast<TextureMTL*>(texture);
    if (!mtlTexture) {
        return false;
    }

    if (index >= m_colorTextures.size()) {
        m_colorTextures.resize(index + 1, nil);
        m_colorMipLevels.resize(index + 1, 0);
    }

    m_colorTextures[index] = mtlTexture->GetTexture();
    m_colorMipLevels[index] = mipLevel;
    
    UpdateRenderPassDescriptor();
    return true;
}

bool FrameBufferMTL::AttachDepthTexture(ITextureImpl* texture, uint32_t mipLevel) {
    TextureMTL* mtlTexture = static_cast<TextureMTL*>(texture);
    if (!mtlTexture) {
        return false;
    }

    m_depthTexture = mtlTexture->GetTexture();
    m_depthMipLevel = mipLevel;
    
    UpdateRenderPassDescriptor();
    return true;
}

bool FrameBufferMTL::AttachStencilTexture(ITextureImpl* texture, uint32_t mipLevel) {
    TextureMTL* mtlTexture = static_cast<TextureMTL*>(texture);
    if (!mtlTexture) {
        return false;
    }

    m_stencilTexture = mtlTexture->GetTexture();
    m_stencilMipLevel = mipLevel;
    
    UpdateRenderPassDescriptor();
    return true;
}

bool FrameBufferMTL::AttachDepthStencilTexture(ITextureImpl* texture, uint32_t mipLevel) {
    TextureMTL* mtlTexture = static_cast<TextureMTL*>(texture);
    if (!mtlTexture) {
        return false;
    }

    m_depthTexture = mtlTexture->GetTexture();
    m_stencilTexture = mtlTexture->GetTexture();
    m_depthMipLevel = mipLevel;
    m_stencilMipLevel = mipLevel;
    
    UpdateRenderPassDescriptor();
    return true;
}

bool FrameBufferMTL::IsComplete() const {
    // Metal中，只要有至少一个颜色附件或深度附件，帧缓冲就是完整的
    return !m_colorTextures.empty() || m_depthTexture != nil;
}

void FrameBufferMTL::Bind() {
    // Metal中，帧缓冲绑定在创建渲染命令编码器时完成
}

void FrameBufferMTL::Unbind() {
    // Metal中，帧缓冲绑定在创建渲染命令编码器时完成
}

void FrameBufferMTL::Clear(uint32_t flags, const float* color, float depth, uint8_t stencil) {
    // 更新清除值
    if (flags & ClearColor && color) {
        memcpy(m_clearColor, color, sizeof(m_clearColor));
    }
    if (flags & ClearDepth) {
        m_clearDepth = depth;
    }
    if (flags & ClearStencil) {
        m_clearStencil = stencil;
    }
    
    // 更新渲染通道描述符中的清除值
    UpdateRenderPassDescriptor();
}

ResourceHandle FrameBufferMTL::GetNativeHandle() const {
    return ResourceHandle((__bridge void*)m_renderPassDescriptor);
}

uint32_t FrameBufferMTL::GetWidth() const {
    return m_width;
}

uint32_t FrameBufferMTL::GetHeight() const {
    return m_height;
}

uint32_t FrameBufferMTL::GetColorAttachmentCount() const {
    return static_cast<uint32_t>(m_colorTextures.size());
}

id<MTLTexture> FrameBufferMTL::GetColorTexture(uint32_t index) const {
    if (index < m_colorTextures.size()) {
        return m_colorTextures[index];
    }
    return nil;
}

void FrameBufferMTL::UpdateRenderPassDescriptor() {
    if (!m_renderPassDescriptor) {
        return;
    }

    // 配置颜色附件
    for (size_t i = 0; i < m_colorTextures.size(); ++i) {
        MTLRenderPassColorAttachmentDescriptor* colorAttachment = m_renderPassDescriptor.colorAttachments[i];
        colorAttachment.texture = m_colorTextures[i];
        colorAttachment.level = m_colorMipLevels[i];
        colorAttachment.loadAction = MTLLoadActionClear;
        colorAttachment.storeAction = MTLStoreActionStore;
        colorAttachment.clearColor = MTLClearColorMake(m_clearColor[0], m_clearColor[1], 
                                                        m_clearColor[2], m_clearColor[3]);
    }

    // 配置深度附件
    if (m_depthTexture) {
        m_renderPassDescriptor.depthAttachment.texture = m_depthTexture;
        m_renderPassDescriptor.depthAttachment.level = m_depthMipLevel;
        m_renderPassDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
        m_renderPassDescriptor.depthAttachment.storeAction = MTLStoreActionStore;
        m_renderPassDescriptor.depthAttachment.clearDepth = m_clearDepth;
    }

    // 配置模板附件
    if (m_stencilTexture) {
        m_renderPassDescriptor.stencilAttachment.texture = m_stencilTexture;
        m_renderPassDescriptor.stencilAttachment.level = m_stencilMipLevel;
        m_renderPassDescriptor.stencilAttachment.loadAction = MTLLoadActionClear;
        m_renderPassDescriptor.stencilAttachment.storeAction = MTLStoreActionStore;
        m_renderPassDescriptor.stencilAttachment.clearStencil = m_clearStencil;
    }
}

} // namespace mtl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
