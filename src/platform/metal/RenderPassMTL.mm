/**
 * @file RenderPassMTL.mm
 * @brief Metal渲染通道实现
 */

#include "RenderPassMTL.h"
#include "FrameBufferMTL.h"
#include "lrengine/utils/LRLog.h"

#ifdef LRENGINE_ENABLE_METAL

namespace lrengine {
namespace render {
namespace mtl {

RenderPassMTL::RenderPassMTL(const RenderPassConfig& config)
    : m_config(config)
    , m_width(0)
    , m_height(0)
{
    CalculateDimensions();
}

RenderPassMTL::~RenderPassMTL() {
}

MTLRenderPassDescriptor* RenderPassMTL::CreateDescriptor() const {
    MTLRenderPassDescriptor* passDesc = [[MTLRenderPassDescriptor alloc] init];
    
    if (m_config.useDefaultFrameBuffer) {
        // 使用默认帧缓冲（屏幕）
        if (!m_config.drawable) {
            LR_LOG_ERROR_F("RenderPassMTL: Default framebuffer requires a valid drawable");
            return nil;
        }
        
        // 设置颜色附件（drawable texture）
        passDesc.colorAttachments[0].texture = m_config.drawable.texture;
        passDesc.colorAttachments[0].loadAction = m_config.colorLoadAction;
        passDesc.colorAttachments[0].storeAction = m_config.colorStoreAction;
        passDesc.colorAttachments[0].clearColor = MTLClearColorMake(
            m_config.clearColor[0], m_config.clearColor[1],
            m_config.clearColor[2], m_config.clearColor[3]
        );
        
        // 设置深度附件（如果有）
        if (m_config.defaultDepthTexture) {
            passDesc.depthAttachment.texture = m_config.defaultDepthTexture;
            passDesc.depthAttachment.loadAction = m_config.depthLoadAction;
            passDesc.depthAttachment.storeAction = m_config.depthStoreAction;
            passDesc.depthAttachment.clearDepth = m_config.clearDepth;
        }
        
        LR_LOG_INFO_F("RenderPassMTL: Created descriptor for default framebuffer (%ux%u)",
                    m_width, m_height);
    } else {
        // 使用离屏渲染目标
        if (!m_config.frameBuffer) {
            LR_LOG_ERROR_F("RenderPassMTL: Offscreen render pass requires a valid frameBuffer");
            return nil;
        }
        
        uint32_t colorAttachmentCount = m_config.frameBuffer->GetColorAttachmentCount();
        if (colorAttachmentCount == 0) {
            LR_LOG_ERROR_F("RenderPassMTL: FrameBuffer has no color attachments");
            return nil;
        }
        
        // 配置多个颜色附件（MRT支持）
        for (uint32_t i = 0; i < colorAttachmentCount; ++i) {
            id<MTLTexture> colorTexture = m_config.frameBuffer->GetColorTexture(i);
            if (!colorTexture) {
                LR_LOG_WARNING_F("RenderPassMTL: Color attachment %u has no texture, skipping", i);
                continue;
            }
            
            MTLRenderPassColorAttachmentDescriptor* colorAttachment = passDesc.colorAttachments[i];
            colorAttachment.texture = colorTexture;
            colorAttachment.loadAction = m_config.colorLoadAction;
            colorAttachment.storeAction = m_config.colorStoreAction;
            colorAttachment.clearColor = MTLClearColorMake(
                m_config.clearColor[0], m_config.clearColor[1],
                m_config.clearColor[2], m_config.clearColor[3]
            );
            
            LR_LOG_INFO_F("RenderPassMTL: Configured color attachment %u (texture: %p)", i, colorTexture);
        }
        
        // 配置深度附件（从同一个FrameBuffer获取）
        id<MTLTexture> depthTexture = m_config.frameBuffer->GetDepthTexture();
        if (depthTexture) {
            passDesc.depthAttachment.texture = depthTexture;
            passDesc.depthAttachment.loadAction = m_config.depthLoadAction;
            passDesc.depthAttachment.storeAction = m_config.depthStoreAction;
            passDesc.depthAttachment.clearDepth = m_config.clearDepth;
            
            LR_LOG_INFO_F("RenderPassMTL: Configured depth attachment (texture: %p)", depthTexture);
        }
        
        LR_LOG_INFO_F("RenderPassMTL: Created descriptor for offscreen rendering (%u color attachments, %ux%u)",
                    colorAttachmentCount, m_width, m_height);
    }
    
    return passDesc;
}

void RenderPassMTL::SetClearColor(float r, float g, float b, float a) {
    m_config.clearColor[0] = r;
    m_config.clearColor[1] = g;
    m_config.clearColor[2] = b;
    m_config.clearColor[3] = a;
}

void RenderPassMTL::SetClearDepth(float depth) {
    m_config.clearDepth = depth;
}

void RenderPassMTL::SetClearStencil(uint8_t stencil) {
    m_config.clearStencil = stencil;
}

void RenderPassMTL::CalculateDimensions() {
    if (m_config.useDefaultFrameBuffer && m_config.drawable) {
        // 从 drawable 获取尺寸
        m_width = static_cast<uint32_t>(m_config.drawable.texture.width);
        m_height = static_cast<uint32_t>(m_config.drawable.texture.height);
    } else if (m_config.frameBuffer) {
        // 从 FrameBuffer 获取尺寸
        m_width = m_config.frameBuffer->GetWidth();
        m_height = m_config.frameBuffer->GetHeight();
    } else {
        LR_LOG_WARNING_F("RenderPassMTL: Cannot determine dimensions, no valid targets");
        m_width = 0;
        m_height = 0;
    }
}

uint32_t RenderPassMTL::GetColorTargetCount() const {
    if (m_config.useDefaultFrameBuffer) {
        return 1;  // 默认帧缓冲只有1个颜色附件
    }
    return m_config.frameBuffer ? m_config.frameBuffer->GetColorAttachmentCount() : 0;
}

} // namespace mtl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
