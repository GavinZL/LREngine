/**
 * @file FrameBufferGLES.cpp
 * @brief OpenGL ES帧缓冲实现
 */

#include "FrameBufferGLES.h"
#include "TextureGLES.h"
#include "lrengine/core/LRError.h"
#include "lrengine/utils/LRLog.h"

#ifdef LRENGINE_ENABLE_OPENGLES

namespace lrengine {
namespace render {
namespace gles {

FrameBufferGLES::FrameBufferGLES()
    : m_fbo(0)
    , m_depthTexture(0)
    , m_depthStencilRBO(0)
    , m_width(0)
    , m_height(0)
    , m_samples(1)
    , m_hasDepthStencil(false)
    , m_depthFormat(PixelFormat::Depth24Stencil8) {}

FrameBufferGLES::~FrameBufferGLES() { Destroy(); }

bool FrameBufferGLES::Create(const FrameBufferDescriptor& desc) {
    if (m_fbo != 0) {
        Destroy();
    }

    m_width           = desc.width;
    m_height          = desc.height;
    m_samples         = desc.samples;
    m_hasDepthStencil = desc.hasDepthStencil;
    m_depthFormat     = desc.depthStencilAttachment.format;

    // 创建帧缓冲对象
    glGenFramebuffers(1, &m_fbo);
    if (m_fbo == 0) {
        LR_SET_ERROR(ErrorCode::FrameBufferIncomplete, "Failed to generate framebuffer");
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // 创建颜色附件
    size_t colorCount = desc.colorAttachments.size();
    if (colorCount > 0) {
        m_colorTextures.resize(colorCount);
        glGenTextures(static_cast<GLsizei>(colorCount), m_colorTextures.data());

        std::vector<GLenum> drawBuffers;
        drawBuffers.reserve(colorCount);

        for (size_t i = 0; i < colorCount; ++i) {
            const ColorAttachmentDescriptor& colorDesc = desc.colorAttachments[i];
            GLuint tex                                 = m_colorTextures[i];

            glBindTexture(GL_TEXTURE_2D, tex);

            GLenum internalFormat = ToGLESInternalFormat(colorDesc.format);
            GLenum format         = ToGLESFormat(colorDesc.format);
            GLenum type           = ToGLESType(colorDesc.format);

            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0, format, type,
                         nullptr);

            // 设置纹理参数
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // 附加到帧缓冲
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i),
                                   GL_TEXTURE_2D, tex, 0);

            drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i));

            LR_LOG_DEBUG_F("OpenGL ES FBO color attachment %zu: tex=%u, format=0x%x", i, tex,
                           internalFormat);
        }

        glBindTexture(GL_TEXTURE_2D, 0);

        // 设置绘制缓冲区
        if (drawBuffers.size() > 1) {
            glDrawBuffers(static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());
        }
    }

    // 创建深度/模板附件
    if (m_hasDepthStencil) {
        // 对于移动平台，使用渲染缓冲区通常更高效
        glGenRenderbuffers(1, &m_depthStencilRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, m_depthStencilRBO);

        GLenum depthInternalFormat = ToGLESInternalFormat(m_depthFormat);

        if (m_samples > 1) {
#ifdef GL_RENDERBUFFER_SAMPLES
            // OpenGL ES 3.0+ 支持多采样渲染缓冲
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_samples, depthInternalFormat,
                                             m_width, m_height);
#else
            glRenderbufferStorage(GL_RENDERBUFFER, depthInternalFormat, m_width, m_height);
#endif
        } else {
            glRenderbufferStorage(GL_RENDERBUFFER, depthInternalFormat, m_width, m_height);
        }

        // 根据格式附加深度和/或模板
        bool hasStencil = HasStencil(m_depthFormat);
        bool hasDepth   = IsDepthFormat(m_depthFormat);

        if (hasDepth && hasStencil) {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                      m_depthStencilRBO);
        } else if (hasDepth) {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                      m_depthStencilRBO);
        } else if (hasStencil) {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                      m_depthStencilRBO);
        }

        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        LR_LOG_DEBUG_F("OpenGL ES FBO depth/stencil: RBO=%u, format=0x%x", m_depthStencilRBO,
                       depthInternalFormat);
    }

    // 检查完整性
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        const char* statusStr = "Unknown";
        switch (status) {
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                statusStr = "INCOMPLETE_ATTACHMENT";
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                statusStr = "INCOMPLETE_MISSING_ATTACHMENT";
                break;
#if LRENGINE_GLES_AVAILABLE
            case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
                statusStr = "INCOMPLETE_DIMENSIONS";
                break;
#endif
            case GL_FRAMEBUFFER_UNSUPPORTED:
                statusStr = "UNSUPPORTED";
                break;
#ifdef GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE
            case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
                statusStr = "INCOMPLETE_MULTISAMPLE";
                break;
#endif
        }

        LR_LOG_ERROR_F("OpenGL ES Framebuffer incomplete: %s (0x%x)", statusStr, status);
        LR_SET_ERROR(ErrorCode::FrameBufferIncomplete, statusStr);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        Destroy();
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    LR_LOG_DEBUG_F("OpenGL ES FBO created: ID=%u, size=%ux%u, colors=%zu, depth=%s", m_fbo, m_width,
                   m_height, m_colorTextures.size(), m_hasDepthStencil ? "yes" : "no");

    return true;
}

void FrameBufferGLES::Destroy() {
    if (!m_colorTextures.empty()) {
        glDeleteTextures(static_cast<GLsizei>(m_colorTextures.size()), m_colorTextures.data());
        m_colorTextures.clear();
    }

    if (m_depthTexture != 0) {
        glDeleteTextures(1, &m_depthTexture);
        m_depthTexture = 0;
    }

    if (m_depthStencilRBO != 0) {
        glDeleteRenderbuffers(1, &m_depthStencilRBO);
        m_depthStencilRBO = 0;
    }

    if (m_fbo != 0) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }

    m_width = m_height = 0;
}

void FrameBufferGLES::Bind() {
    if (m_fbo != 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        glViewport(0, 0, m_width, m_height);
    }
}

void FrameBufferGLES::Unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

bool FrameBufferGLES::IsComplete() const {
    if (m_fbo == 0) {
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return status == GL_FRAMEBUFFER_COMPLETE;
}

ResourceHandle FrameBufferGLES::GetNativeHandle() const {
    ResourceHandle handle;
    handle.glHandle = m_fbo;
    return handle;
}

uint32_t FrameBufferGLES::GetWidth() const { return m_width; }
uint32_t FrameBufferGLES::GetHeight() const { return m_height; }

GLuint FrameBufferGLES::GetColorTexture(uint32_t index) const {
    if (index < m_colorTextures.size()) {
        return m_colorTextures[index];
    }
    return 0;
}

void FrameBufferGLES::Invalidate(bool colorInvalid, bool depthInvalid, bool stencilInvalid) {
    // glInvalidateFramebuffer是移动GPU的重要优化
    // 它告诉驱动程序可以丢弃帧缓冲内容，避免不必要的内存操作

    std::vector<GLenum> attachments;

    if (colorInvalid) {
        for (size_t i = 0; i < m_colorTextures.size(); ++i) {
            attachments.push_back(GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i));
        }
    }

    if (depthInvalid) {
        attachments.push_back(GL_DEPTH_ATTACHMENT);
    }

    if (stencilInvalid) {
        attachments.push_back(GL_STENCIL_ATTACHMENT);
    }

    if (!attachments.empty()) {
#if LRENGINE_GLES_AVAILABLE
        glInvalidateFramebuffer(GL_FRAMEBUFFER, static_cast<GLsizei>(attachments.size()),
                                attachments.data());
#endif
    }
}

bool FrameBufferGLES::AttachColorTexture(ITextureImpl* texture, uint32_t index, uint32_t mipLevel) {
    if (m_fbo == 0 || texture == nullptr) {
        return false;
    }

    TextureGLES* texGLES = dynamic_cast<TextureGLES*>(texture);
    if (!texGLES) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Invalid texture type");
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, texGLES->GetTarget(),
                           texGLES->GetTextureID(), mipLevel);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

bool FrameBufferGLES::AttachDepthTexture(ITextureImpl* texture, uint32_t mipLevel) {
    if (m_fbo == 0 || texture == nullptr) {
        return false;
    }

    TextureGLES* texGLES = dynamic_cast<TextureGLES*>(texture);
    if (!texGLES) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Invalid texture type");
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texGLES->GetTarget(),
                           texGLES->GetTextureID(), mipLevel);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    m_depthTexture = texGLES->GetTextureID();
    return true;
}

bool FrameBufferGLES::AttachStencilTexture(ITextureImpl* texture, uint32_t mipLevel) {
    if (m_fbo == 0 || texture == nullptr) {
        return false;
    }

    TextureGLES* texGLES = dynamic_cast<TextureGLES*>(texture);
    if (!texGLES) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Invalid texture type");
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, texGLES->GetTarget(),
                           texGLES->GetTextureID(), mipLevel);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

bool FrameBufferGLES::AttachDepthStencilTexture(ITextureImpl* texture, uint32_t mipLevel) {
    if (m_fbo == 0 || texture == nullptr) {
        return false;
    }

    TextureGLES* texGLES = dynamic_cast<TextureGLES*>(texture);
    if (!texGLES) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Invalid texture type");
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, texGLES->GetTarget(),
                           texGLES->GetTextureID(), mipLevel);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    m_depthTexture = texGLES->GetTextureID();
    return true;
}

void FrameBufferGLES::Clear(uint32_t flags, const float* color, float depth, uint8_t stencil) {
    GLbitfield clearMask = 0;

    if ((flags & ClearColor) && color != nullptr) {
        glClearColor(color[0], color[1], color[2], color[3]);
        clearMask |= GL_COLOR_BUFFER_BIT;
    }

    if (flags & ClearDepth) {
        glClearDepthf(depth); // OpenGL ES使用glClearDepthf
        glDepthMask(GL_TRUE); // 确保深度写入启用
        clearMask |= GL_DEPTH_BUFFER_BIT;
    }

    if (flags & ClearStencil) {
        glClearStencil(stencil);
        glStencilMask(0xFF); // 确保模板写入启用
        clearMask |= GL_STENCIL_BUFFER_BIT;
    }

    if (clearMask != 0) {
        glClear(clearMask);
    }
}

} // namespace gles
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGLES
