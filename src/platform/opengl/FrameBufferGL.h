/**
 * @file FrameBufferGL.h
 * @brief OpenGL帧缓冲实现
 */

#pragma once

#include "platform/interface/IFrameBufferImpl.h"
#include "TypeConverterGL.h"
#include <vector>

#ifdef LRENGINE_ENABLE_OPENGL

namespace lrengine {
namespace render {
namespace gl {

class TextureGL;

/**
 * @brief OpenGL帧缓冲实现
 */
class FrameBufferGL : public IFrameBufferImpl {
public:
    FrameBufferGL();
    ~FrameBufferGL() override;

    // IFrameBufferImpl接口
    bool Create(const FrameBufferDescriptor& desc) override;
    void Destroy() override;
    bool AttachColorTexture(ITextureImpl* texture, uint32_t index, uint32_t mipLevel) override;
    bool AttachDepthTexture(ITextureImpl* texture, uint32_t mipLevel) override;
    bool AttachStencilTexture(ITextureImpl* texture, uint32_t mipLevel) override;
    bool AttachDepthStencilTexture(ITextureImpl* texture, uint32_t mipLevel) override;
    bool IsComplete() const override;
    void Bind() override;
    void Unbind() override;
    void Clear(uint32_t clearFlags,
               const float* clearColor,
               float clearDepth,
               uint8_t clearStencil) override;
    ResourceHandle GetNativeHandle() const override;
    uint32_t GetWidth() const override;
    uint32_t GetHeight() const override;
    uint32_t GetColorAttachmentCount() const override;

    // OpenGL特有方法
    GLuint GetFrameBufferID() const { return m_fboID; }
    void SetDrawBuffers();

private:
    GLuint m_fboID;
    uint32_t m_width;
    uint32_t m_height;
    std::vector<GLenum> m_drawBuffers;
    bool m_hasDepth;
    bool m_hasStencil;
};

/**
 * @brief 默认帧缓冲包装器（用于渲染到屏幕）
 */
class DefaultFrameBufferGL : public IFrameBufferImpl {
public:
    DefaultFrameBufferGL(uint32_t width, uint32_t height);
    ~DefaultFrameBufferGL() override = default;

    bool Create(const FrameBufferDescriptor& desc) override;
    void Destroy() override;
    bool AttachColorTexture(ITextureImpl* texture, uint32_t index, uint32_t mipLevel) override;
    bool AttachDepthTexture(ITextureImpl* texture, uint32_t mipLevel) override;
    bool AttachStencilTexture(ITextureImpl* texture, uint32_t mipLevel) override;
    bool AttachDepthStencilTexture(ITextureImpl* texture, uint32_t mipLevel) override;
    bool IsComplete() const override;
    void Bind() override;
    void Unbind() override;
    void Clear(uint32_t clearFlags,
               const float* clearColor,
               float clearDepth,
               uint8_t clearStencil) override;
    ResourceHandle GetNativeHandle() const override;
    uint32_t GetWidth() const override;
    uint32_t GetHeight() const override;
    uint32_t GetColorAttachmentCount() const override;

    void Resize(uint32_t width, uint32_t height);

private:
    uint32_t m_width;
    uint32_t m_height;
};

} // namespace gl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGL
