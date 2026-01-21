/**
 * @file FrameBufferGLES.h
 * @brief OpenGL ES帧缓冲实现
 */

#pragma once

#include "platform/interface/IFrameBufferImpl.h"
#include "TypeConverterGLES.h"
#include <vector>

#ifdef LRENGINE_ENABLE_OPENGLES

namespace lrengine {
namespace render {
namespace gles {

/**
 * @brief OpenGL ES帧缓冲实现
 */
class FrameBufferGLES : public IFrameBufferImpl {
public:
    FrameBufferGLES();
    ~FrameBufferGLES() override;

    // IFrameBufferImpl接口
    bool Create(const FrameBufferDescriptor& desc) override;
    void Destroy() override;
    void Bind() override;
    void Unbind() override;
    bool IsComplete() const override;
    ResourceHandle GetNativeHandle() const override;
    uint32_t GetWidth() const override;
    uint32_t GetHeight() const override;

    // 纹理附加接口
    bool AttachColorTexture(ITextureImpl* texture, uint32_t index, uint32_t mipLevel = 0) override;
    bool AttachDepthTexture(ITextureImpl* texture, uint32_t mipLevel = 0) override;
    bool AttachStencilTexture(ITextureImpl* texture, uint32_t mipLevel = 0) override;
    bool AttachDepthStencilTexture(ITextureImpl* texture, uint32_t mipLevel = 0) override;
    void Clear(uint32_t flags, const float* color, float depth, uint8_t stencil) override;
    uint32_t GetColorAttachmentCount() const override {
        return static_cast<uint32_t>(m_colorTextures.size());
    }

    // OpenGL ES特有方法
    GLuint GetFrameBufferID() const { return m_fbo; }
    GLuint GetColorTexture(uint32_t index) const;
    GLuint GetDepthTexture() const { return m_depthTexture; }
    GLuint GetDepthStencilRenderbuffer() const { return m_depthStencilRBO; }

    /**
     * @brief 使帧缓冲内容失效（移动GPU优化）
     * @param colorInvalid 是否使颜色附件失效
     * @param depthInvalid 是否使深度附件失效
     * @param stencilInvalid 是否使模板附件失效
     */
    void Invalidate(bool colorInvalid, bool depthInvalid, bool stencilInvalid);

private:
    GLuint m_fbo;
    std::vector<GLuint> m_colorTextures;
    GLuint m_depthTexture;
    GLuint m_depthStencilRBO; // 深度模板渲染缓冲
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_samples;
    bool m_hasDepthStencil;
    PixelFormat m_depthFormat;
};

} // namespace gles
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGLES
