/**
 * @file TextureGL.cpp
 * @brief OpenGL纹理实现
 */

#include "TextureGL.h"
#include "lrengine/core/LRError.h"
#include "lrengine/utils/LRLog.h"

#include <algorithm>

#ifdef LRENGINE_ENABLE_OPENGL

// 各向异性过滤扩展常量（如果未定义）
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

namespace lrengine {
namespace render {
namespace gl {

// 采样器参数转换辅助函数
static GLenum ToGLFilterMode(FilterMode mode, bool mipmap = false) {
    switch (mode) {
        case FilterMode::Nearest:
            return mipmap ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
        case FilterMode::Linear:
            return mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
        default:
            return GL_LINEAR;
    }
}

static GLenum ToGLWrapMode(WrapMode mode) {
    switch (mode) {
        case WrapMode::Repeat:
            return GL_REPEAT;
        case WrapMode::MirroredRepeat:
            return GL_MIRRORED_REPEAT;
        case WrapMode::ClampToEdge:
            return GL_CLAMP_TO_EDGE;
        case WrapMode::ClampToBorder:
            return GL_CLAMP_TO_BORDER;
        default:
            return GL_REPEAT;
    }
}

// ============================================================================
// TextureGL
// ============================================================================

TextureGL::TextureGL()
    : m_textureID(0)
    , m_target(GL_TEXTURE_2D)
    , m_internalFormat(GL_RGBA8)
    , m_format(GL_RGBA)
    , m_dataType(GL_UNSIGNED_BYTE)
    , m_width(0)
    , m_height(0)
    , m_depth(1)
    , m_mipLevels(1)
    , m_type(TextureType::Texture2D)
    , m_pixelFormat(PixelFormat::RGBA8) {}

TextureGL::~TextureGL() { Destroy(); }

bool TextureGL::Create(const TextureDescriptor& desc) {
    if (m_textureID != 0) {
        Destroy();
    }

    m_type        = desc.type;
    m_pixelFormat = desc.format;
    m_width       = desc.width;
    m_height      = desc.height;
    m_depth       = desc.depth;
    m_mipLevels   = desc.mipLevels;

    m_target         = ToGLTextureTarget(desc.type);
    m_internalFormat = ToGLInternalFormat(desc.format);
    m_format         = ToGLFormat(desc.format);
    m_dataType       = ToGLType(desc.format);

    glGenTextures(1, &m_textureID);
    if (m_textureID == 0) {
        LR_SET_ERROR(ErrorCode::TextureCreationFailed, "Failed to generate texture");
        return false;
    }

    glBindTexture(m_target, m_textureID);

    // 根据纹理类型创建存储
    switch (m_type) {
        case TextureType::Texture2D:
            // 为每个mip level分配存储（GL 3.3兼容方式）
            for (uint32_t level = 0; level < m_mipLevels; ++level) {
                uint32_t mipWidth     = std::max(1u, m_width >> level);
                uint32_t mipHeight    = std::max(1u, m_height >> level);
                const void* levelData = (level == 0) ? desc.data : nullptr;
                glTexImage2D(m_target, level, m_internalFormat, mipWidth, mipHeight, 0, m_format,
                             m_dataType, levelData);
                LR_LOG_DEBUG_F("OpenGL Create Texture: %d, Level: %d, Width: %d, Height: %d, "
                               "data:%p",
                               m_textureID, level, mipWidth, mipHeight, desc.data);
            }
            break;

        case TextureType::Texture3D:
            for (uint32_t level = 0; level < m_mipLevels; ++level) {
                uint32_t mipWidth     = std::max(1u, m_width >> level);
                uint32_t mipHeight    = std::max(1u, m_height >> level);
                uint32_t mipDepth     = std::max(1u, m_depth >> level);
                const void* levelData = (level == 0) ? desc.data : nullptr;
                glTexImage3D(m_target, level, m_internalFormat, mipWidth, mipHeight, mipDepth, 0,
                             m_format, m_dataType, levelData);
            }
            break;

        case TextureType::TextureCube:
            // CubeMap需要为每个面单独设置
            for (uint32_t level = 0; level < m_mipLevels; ++level) {
                uint32_t mipWidth  = std::max(1u, m_width >> level);
                uint32_t mipHeight = std::max(1u, m_height >> level);
                for (uint32_t face = 0; face < 6; ++face) {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level, m_internalFormat,
                                 mipWidth, mipHeight, 0, m_format, m_dataType, nullptr);
                }
            }
            break;

        case TextureType::Texture2DArray:
            for (uint32_t level = 0; level < m_mipLevels; ++level) {
                uint32_t mipWidth     = std::max(1u, m_width >> level);
                uint32_t mipHeight    = std::max(1u, m_height >> level);
                const void* levelData = (level == 0) ? desc.data : nullptr;
                glTexImage3D(m_target, level, m_internalFormat, mipWidth, mipHeight, m_depth, 0,
                             m_format, m_dataType, levelData);
            }
            break;

        case TextureType::Texture2DMultisample:
            glTexImage2DMultisample(m_target, desc.sampleCount, m_internalFormat, m_width, m_height,
                                    GL_TRUE);
            break;

        default:
            LR_SET_ERROR(ErrorCode::InvalidArgument, "Unsupported texture type");
            glDeleteTextures(1, &m_textureID);
            m_textureID = 0;
            return false;
    }

    // 设置采样器参数
    SetSamplerParameters(desc);

    glBindTexture(m_target, 0);
    return true;
}

void TextureGL::SetSamplerParameters(const TextureDescriptor& desc) {
    // 多采样纹理不支持采样器参数
    if (m_type == TextureType::Texture2DMultisample) {
        return;
    }

    bool hasMipmaps = m_mipLevels > 1;

    glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER,
                    ToGLFilterMode(desc.sampler.minFilter, hasMipmaps));
    glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, ToGLFilterMode(desc.sampler.magFilter, false));
    glTexParameteri(m_target, GL_TEXTURE_WRAP_S, ToGLWrapMode(desc.sampler.wrapU));
    glTexParameteri(m_target, GL_TEXTURE_WRAP_T, ToGLWrapMode(desc.sampler.wrapV));

    if (m_type == TextureType::Texture3D || m_type == TextureType::TextureCube) {
        glTexParameteri(m_target, GL_TEXTURE_WRAP_R, ToGLWrapMode(desc.sampler.wrapW));
    }

    // 各向异性过滤
    if (desc.sampler.maxAnisotropy > 1.0f) {
        GLfloat maxAniso;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
        float aniso = std::min(desc.sampler.maxAnisotropy, maxAniso);
        glTexParameterf(m_target, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
    }

    // 比较模式（用于阴影贴图）
    if (desc.sampler.compareMode) {
        glTexParameteri(m_target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(m_target, GL_TEXTURE_COMPARE_FUNC,
                        ToGLCompareFunc(static_cast<CompareFunc>(desc.sampler.compareFuncValue)));
    }

    // 边界颜色
    if (desc.sampler.wrapU == WrapMode::ClampToBorder ||
        desc.sampler.wrapV == WrapMode::ClampToBorder ||
        desc.sampler.wrapW == WrapMode::ClampToBorder) {
        glTexParameterfv(m_target, GL_TEXTURE_BORDER_COLOR, desc.sampler.borderColor);
    }
}

void TextureGL::Destroy() {
    if (m_textureID != 0) {
        glDeleteTextures(1, &m_textureID);
        m_textureID = 0;
    }
    m_width = m_height = 0;
    m_depth            = 1;
}

void TextureGL::UpdateData(const void* data, uint32_t mipLevel, const TextureRegion* region) {
    if (m_textureID == 0 || data == nullptr) {
        return;
    }

    if (mipLevel >= m_mipLevels) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Invalid mip level");
        return;
    }

    glBindTexture(m_target, m_textureID);

    if (region != nullptr) {
        // 更新子区域
        switch (m_type) {
            case TextureType::Texture2D:
                glTexSubImage2D(m_target, mipLevel, region->x, region->y, region->width,
                                region->height, m_format, m_dataType, data);
                break;

            case TextureType::Texture3D:
            case TextureType::Texture2DArray:
                glTexSubImage3D(m_target, mipLevel, region->x, region->y, region->z, region->width,
                                region->height, region->depth, m_format, m_dataType, data);
                break;

            case TextureType::TextureCube:
                glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + region->z, mipLevel, region->x,
                                region->y, region->width, region->height, m_format, m_dataType, data);
                break;

            default:
                break;
        }
    } else {
        // 更新整个mip level
        uint32_t mipWidth  = std::max(1u, m_width >> mipLevel);
        uint32_t mipHeight = std::max(1u, m_height >> mipLevel);
        uint32_t mipDepth  = std::max(1u, m_depth >> mipLevel);

        switch (m_type) {
            case TextureType::Texture2D:
                glTexSubImage2D(m_target, mipLevel, 0, 0, mipWidth, mipHeight, m_format, m_dataType,
                                data);
                break;

            case TextureType::Texture3D:
            case TextureType::Texture2DArray:
                glTexSubImage3D(m_target, mipLevel, 0, 0, 0, mipWidth, mipHeight, mipDepth,
                                m_format, m_dataType, data);
                break;

            default:
                break;
        }
    }

    glBindTexture(m_target, 0);
}

void TextureGL::GenerateMipmaps() {
    if (m_textureID == 0 || m_mipLevels <= 1) {
        return;
    }

    // 多采样纹理不支持mipmap
    if (m_type == TextureType::Texture2DMultisample) {
        return;
    }

    glBindTexture(m_target, m_textureID);
    glGenerateMipmap(m_target);
    glBindTexture(m_target, 0);
}

void TextureGL::Bind(uint32_t slot) {
    if (m_textureID != 0) {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(m_target, m_textureID);
        LR_LOG_DEBUG_F("OpenGL Bind Texture: %d", m_textureID);
    }
}

void TextureGL::Unbind(uint32_t slot) {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(m_target, 0);
}

ResourceHandle TextureGL::GetNativeHandle() const {
    ResourceHandle handle;
    handle.glHandle = m_textureID;
    return handle;
}

uint32_t TextureGL::GetWidth() const { return m_width; }
uint32_t TextureGL::GetHeight() const { return m_height; }
uint32_t TextureGL::GetDepth() const { return m_depth; }
TextureType TextureGL::GetType() const { return m_type; }
PixelFormat TextureGL::GetFormat() const { return m_pixelFormat; }

// ============================================================================
// SamplerGL
// ============================================================================

SamplerGL::SamplerGL() : m_samplerID(0) {}

SamplerGL::~SamplerGL() { Destroy(); }

bool SamplerGL::Create(const SamplerDescriptor& desc) {
    if (m_samplerID != 0) {
        Destroy();
    }

    glGenSamplers(1, &m_samplerID);
    if (m_samplerID == 0) {
        return false;
    }

    glSamplerParameteri(m_samplerID, GL_TEXTURE_MIN_FILTER, ToGLFilterMode(desc.minFilter, true));
    glSamplerParameteri(m_samplerID, GL_TEXTURE_MAG_FILTER, ToGLFilterMode(desc.magFilter, false));
    glSamplerParameteri(m_samplerID, GL_TEXTURE_WRAP_S, ToGLWrapMode(desc.wrapU));
    glSamplerParameteri(m_samplerID, GL_TEXTURE_WRAP_T, ToGLWrapMode(desc.wrapV));
    glSamplerParameteri(m_samplerID, GL_TEXTURE_WRAP_R, ToGLWrapMode(desc.wrapW));

    if (desc.maxAnisotropy > 1.0f) {
        GLfloat maxAniso;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
        float aniso = std::min(desc.maxAnisotropy, maxAniso);
        glSamplerParameterf(m_samplerID, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
    }

    if (desc.compareMode) {
        glSamplerParameteri(m_samplerID, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glSamplerParameteri(m_samplerID, GL_TEXTURE_COMPARE_FUNC,
                            ToGLCompareFunc(static_cast<CompareFunc>(desc.compareFuncValue)));
    }

    glSamplerParameterfv(m_samplerID, GL_TEXTURE_BORDER_COLOR, desc.borderColor);

    return true;
}

void SamplerGL::Destroy() {
    if (m_samplerID != 0) {
        glDeleteSamplers(1, &m_samplerID);
        m_samplerID = 0;
    }
}

void SamplerGL::Bind(uint32_t slot) {
    if (m_samplerID != 0) {
        glBindSampler(slot, m_samplerID);
    }
}

void SamplerGL::Unbind(uint32_t slot) { glBindSampler(slot, 0); }

} // namespace gl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGL
