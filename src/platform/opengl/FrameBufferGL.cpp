/**
 * @file FrameBufferGL.cpp
 * @brief OpenGL帧缓冲实现
 */

#include "FrameBufferGL.h"
#include "TextureGL.h"
#include "lrengine/core/LRError.h"
#include "lrengine/utils/LRLog.h"
#ifdef LRENGINE_ENABLE_OPENGL

namespace lrengine {
namespace render {
namespace gl {

// ============================================================================
// FrameBufferGL
// ============================================================================

FrameBufferGL::FrameBufferGL()
    : m_fboID(0)
    , m_width(0)
    , m_height(0)
    , m_hasDepth(false)
    , m_hasStencil(false)
{
}

FrameBufferGL::~FrameBufferGL()
{
    Destroy();
}

bool FrameBufferGL::Create(const FrameBufferDescriptor& desc)
{
    if (m_fboID != 0) {
        Destroy();
    }

    m_width = desc.width;
    m_height = desc.height;

    glGenFramebuffers(1, &m_fboID);
    if (m_fboID == 0) {
        LR_SET_ERROR(ErrorCode::FrameBufferIncomplete, "Failed to create framebuffer");
        return false;
    }

    LR_LOG_DEBUG_F("OpenGL Create Framebuffer: %d", m_fboID);
    return true;
}

void FrameBufferGL::Destroy()
{
    if (m_fboID != 0) {
        glDeleteFramebuffers(1, &m_fboID);
        m_fboID = 0;
    }
    m_drawBuffers.clear();
    m_width = m_height = 0;
    m_hasDepth = m_hasStencil = false;
}

bool FrameBufferGL::AttachColorTexture(ITextureImpl* texture, uint32_t index, uint32_t mipLevel)
{
    if (m_fboID == 0 || texture == nullptr || index >= 8) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Invalid texture or index");
        return false;
    }

    TextureGL* glTexture = static_cast<TextureGL*>(texture);
    
    glBindFramebuffer(GL_FRAMEBUFFER, m_fboID);

    GLenum attachment = GL_COLOR_ATTACHMENT0 + index;
    
    if (glTexture->GetType() == TextureType::TextureCube) {
        // CubeMap需要特殊处理
        glFramebufferTexture(GL_FRAMEBUFFER, attachment, glTexture->GetTextureID(), mipLevel);
    } else {
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, glTexture->GetTarget(),
                              glTexture->GetTextureID(), mipLevel);
    }

    // 更新绘制缓冲区列表
    if (index >= m_drawBuffers.size()) {
        m_drawBuffers.resize(index + 1, GL_NONE);
    }
    m_drawBuffers[index] = attachment;

    LR_LOG_DEBUG_F("OpenGL Attach Color Texture: %d, Attachment: %d, FBO: %d", glTexture->GetTextureID(), attachment, m_fboID);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

bool FrameBufferGL::AttachDepthTexture(ITextureImpl* texture, uint32_t mipLevel)
{
    if (m_fboID == 0 || texture == nullptr) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Invalid texture");
        return false;
    }

    TextureGL* glTexture = static_cast<TextureGL*>(texture);
    
    glBindFramebuffer(GL_FRAMEBUFFER, m_fboID);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, glTexture->GetTarget(),
                          glTexture->GetTextureID(), mipLevel);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    LR_LOG_DEBUG_F("OpenGL Attach Depth Texture: %d, FBO: %d", glTexture->GetTextureID(), m_fboID);
    m_hasDepth = true;
    return true;
}

bool FrameBufferGL::AttachStencilTexture(ITextureImpl* texture, uint32_t mipLevel)
{
    if (m_fboID == 0 || texture == nullptr) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Invalid texture");
        return false;
    }

    TextureGL* glTexture = static_cast<TextureGL*>(texture);
    
    glBindFramebuffer(GL_FRAMEBUFFER, m_fboID);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, glTexture->GetTarget(),
                          glTexture->GetTextureID(), mipLevel);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    m_hasStencil = true;
    return true;
}

bool FrameBufferGL::AttachDepthStencilTexture(ITextureImpl* texture, uint32_t mipLevel)
{
    if (m_fboID == 0 || texture == nullptr) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Invalid texture");
        return false;
    }

    TextureGL* glTexture = static_cast<TextureGL*>(texture);
    
    glBindFramebuffer(GL_FRAMEBUFFER, m_fboID);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, glTexture->GetTarget(),
                          glTexture->GetTextureID(), mipLevel);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    m_hasDepth = true;
    m_hasStencil = true;
    return true;
}

bool FrameBufferGL::IsComplete() const
{
    if (m_fboID == 0) {
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_fboID);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return status == GL_FRAMEBUFFER_COMPLETE;
}

void FrameBufferGL::Bind()
{
    if (m_fboID != 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_fboID);
        SetDrawBuffers();
        glViewport(0, 0, m_width, m_height);
        LR_LOG_DEBUG_F("OpenGL Bind Framebuffer: FBO=%d, Viewport=%ux%u", m_fboID, m_width, m_height);
    }
}

void FrameBufferGL::Unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBufferGL::SetDrawBuffers()
{
    if (!m_drawBuffers.empty()) {
        glDrawBuffers(static_cast<GLsizei>(m_drawBuffers.size()), m_drawBuffers.data());
    }
}

void FrameBufferGL::Clear(uint32_t clearFlags, const float* clearColor, float clearDepth, uint8_t clearStencil)
{
    GLbitfield mask = 0;

    if (clearFlags & ClearFlag_Color) {
        if (clearColor != nullptr) {
            glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
        }
        mask |= GL_COLOR_BUFFER_BIT;
    }

    if ((clearFlags & ClearFlag_Depth) && m_hasDepth) {
        glClearDepth(clearDepth);
        mask |= GL_DEPTH_BUFFER_BIT;
    }

    if ((clearFlags & ClearFlag_Stencil) && m_hasStencil) {
        glClearStencil(clearStencil);
        mask |= GL_STENCIL_BUFFER_BIT;
    }

    if (mask != 0) {
        glClear(mask);
    }
}

ResourceHandle FrameBufferGL::GetNativeHandle() const
{
    ResourceHandle handle;
    handle.glHandle = m_fboID;
    return handle;
}

uint32_t FrameBufferGL::GetWidth() const { return m_width; }
uint32_t FrameBufferGL::GetHeight() const { return m_height; }
uint32_t FrameBufferGL::GetColorAttachmentCount() const { return static_cast<uint32_t>(m_drawBuffers.size()); }

// ============================================================================
// DefaultFrameBufferGL
// ============================================================================

DefaultFrameBufferGL::DefaultFrameBufferGL(uint32_t width, uint32_t height)
    : m_width(width)
    , m_height(height)
{
}

bool DefaultFrameBufferGL::Create(const FrameBufferDescriptor& desc)
{
    m_width = desc.width;
    m_height = desc.height;
    return true;
}

void DefaultFrameBufferGL::Destroy()
{
    // 默认帧缓冲不需要销毁
}

bool DefaultFrameBufferGL::AttachColorTexture(ITextureImpl*, uint32_t, uint32_t)
{
    // 默认帧缓冲不支持附加纹理
    return false;
}

bool DefaultFrameBufferGL::AttachDepthTexture(ITextureImpl*, uint32_t)
{
    return false;
}

bool DefaultFrameBufferGL::AttachStencilTexture(ITextureImpl*, uint32_t)
{
    return false;
}

bool DefaultFrameBufferGL::AttachDepthStencilTexture(ITextureImpl*, uint32_t)
{
    return false;
}

bool DefaultFrameBufferGL::IsComplete() const
{
    return true;
}

void DefaultFrameBufferGL::Bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_width, m_height);
}

void DefaultFrameBufferGL::Unbind()
{
    // 已经是默认帧缓冲
}

void DefaultFrameBufferGL::Clear(uint32_t clearFlags, const float* clearColor, float clearDepth, uint8_t clearStencil)
{
    GLbitfield mask = 0;

    if (clearFlags & ClearFlag_Color) {
        if (clearColor != nullptr) {
            glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
        }
        mask |= GL_COLOR_BUFFER_BIT;
    }

    if (clearFlags & ClearFlag_Depth) {
        glClearDepth(clearDepth);
        mask |= GL_DEPTH_BUFFER_BIT;
    }

    if (clearFlags & ClearFlag_Stencil) {
        glClearStencil(clearStencil);
        mask |= GL_STENCIL_BUFFER_BIT;
    }

    if (mask != 0) {
        glClear(mask);
    }
}

ResourceHandle DefaultFrameBufferGL::GetNativeHandle() const
{
    ResourceHandle handle;
    handle.glHandle = 0;
    return handle;
}

uint32_t DefaultFrameBufferGL::GetWidth() const { return m_width; }
uint32_t DefaultFrameBufferGL::GetHeight() const { return m_height; }
uint32_t DefaultFrameBufferGL::GetColorAttachmentCount() const { return 1; }

void DefaultFrameBufferGL::Resize(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;
}

} // namespace gl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGL
