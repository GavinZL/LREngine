/**
 * @file TextureGLES.cpp
 * @brief OpenGL ES纹理实现
 */

#include "TextureGLES.h"
#include "lrengine/core/LRError.h"
#include "lrengine/utils/LRLog.h"

#include <algorithm>

#ifdef LRENGINE_ENABLE_OPENGLES

// 各向异性过滤扩展常量
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

namespace lrengine {
namespace render {
namespace gles {

// ============================================================================
// TextureGLES
// ============================================================================

TextureGLES::TextureGLES()
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

TextureGLES::~TextureGLES() { Destroy(); }

bool TextureGLES::Create(const TextureDescriptor& desc) {
    if (m_textureID != 0) {
        Destroy();
    }

    m_type        = desc.type;
    m_pixelFormat = desc.format;
    m_width       = desc.width;
    m_height      = desc.height;
    m_depth       = desc.depth;
    m_mipLevels   = desc.mipLevels;

    m_target         = ToGLESTextureTarget(desc.type);
    m_internalFormat = ToGLESInternalFormat(desc.format);
    m_format         = ToGLESFormat(desc.format);
    m_dataType       = ToGLESType(desc.format);

    glGenTextures(1, &m_textureID);
    if (m_textureID == 0) {
        LR_SET_ERROR(ErrorCode::TextureCreationFailed, "Failed to generate texture");
        return false;
    }

    glBindTexture(m_target, m_textureID);

    // 根据纹理类型创建存储
    switch (m_type) {
        case TextureType::Texture2D:
            // 为每个mip level分配存储
            for (uint32_t level = 0; level < m_mipLevels; ++level) {
                uint32_t mipWidth     = std::max(1u, m_width >> level);
                uint32_t mipHeight    = std::max(1u, m_height >> level);
                const void* levelData = (level == 0) ? desc.data : nullptr;
                glTexImage2D(m_target, level, m_internalFormat, mipWidth, mipHeight, 0, m_format,
                             m_dataType, levelData);
                LR_LOG_DEBUG_F("OpenGL ES Create Texture: %d, Level: %d, Width: %d, Height: %d",
                               m_textureID, level, mipWidth, mipHeight);
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
#if LRENGINE_GLES_AVAILABLE && defined(GL_TEXTURE_2D_MULTISAMPLE)
            // OpenGL ES 3.1+ 支持多采样纹理
            glTexStorage2DMultisample(m_target, desc.sampleCount, m_internalFormat, m_width,
                                      m_height, GL_TRUE);
#elif !LRENGINE_GLES_AVAILABLE
            // 桌面OpenGL使用glTexImage2DMultisample
            glTexImage2DMultisample(m_target, desc.sampleCount, m_internalFormat, m_width, m_height,
                                    GL_TRUE);
#else
            LR_SET_ERROR(ErrorCode::NotSupported, "Multisample textures require OpenGL ES 3.1+");
            glDeleteTextures(1, &m_textureID);
            m_textureID = 0;
            return false;
#endif
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

void TextureGLES::SetSamplerParameters(const TextureDescriptor& desc) {
    // 多采样纹理不支持采样器参数
    if (m_type == TextureType::Texture2DMultisample) {
        return;
    }

    bool hasMipmaps = m_mipLevels > 1;

    glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER,
                    ToGLESFilterMode(desc.sampler.minFilter, hasMipmaps));
    glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, ToGLESFilterMode(desc.sampler.magFilter, false));

    // 包裹模式
    // 注意：GL_CLAMP_TO_BORDER在ES中需要扩展支持
    glTexParameteri(m_target, GL_TEXTURE_WRAP_S, ToGLESWrapMode(desc.sampler.wrapU, false));
    glTexParameteri(m_target, GL_TEXTURE_WRAP_T, ToGLESWrapMode(desc.sampler.wrapV, false));

    if (m_type == TextureType::Texture3D || m_type == TextureType::TextureCube) {
        glTexParameteri(m_target, GL_TEXTURE_WRAP_R, ToGLESWrapMode(desc.sampler.wrapW, false));
    }

    // 各向异性过滤（需要扩展支持）
    if (desc.sampler.maxAnisotropy > 1.0f) {
        GLfloat maxAniso = 1.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
        if (maxAniso > 1.0f) {
            float aniso = std::min(desc.sampler.maxAnisotropy, maxAniso);
            glTexParameterf(m_target, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
        }
    }

    // 比较模式（用于阴影贴图）
    if (desc.sampler.compareMode) {
        glTexParameteri(m_target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(m_target, GL_TEXTURE_COMPARE_FUNC,
                        ToGLESCompareFunc(static_cast<CompareFunc>(desc.sampler.compareFuncValue)));
    }

    // 边界颜色（需要GL_EXT_texture_border_clamp扩展）
    // OpenGL ES默认不支持GL_CLAMP_TO_BORDER，所以这里跳过边界颜色设置
}

void TextureGLES::Destroy() {
    if (m_textureID != 0) {
        glDeleteTextures(1, &m_textureID);
        m_textureID = 0;
    }
    m_width = m_height = 0;
    m_depth            = 1;
}

void TextureGLES::UpdateData(const void* data, uint32_t mipLevel, const TextureRegion* region) {
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

void TextureGLES::GenerateMipmaps() {
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

void TextureGLES::Bind(uint32_t slot) {
    if (m_textureID != 0) {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(m_target, m_textureID);
        LR_LOG_DEBUG_F("OpenGL ES Bind Texture: %d, slot=%u", m_textureID, slot);
    }
}

void TextureGLES::Unbind(uint32_t slot) {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(m_target, 0);
}

ResourceHandle TextureGLES::GetNativeHandle() const {
    ResourceHandle handle;
    handle.glHandle = m_textureID;
    return handle;
}

uint32_t TextureGLES::GetWidth() const { return m_width; }
uint32_t TextureGLES::GetHeight() const { return m_height; }
uint32_t TextureGLES::GetDepth() const { return m_depth; }
TextureType TextureGLES::GetType() const { return m_type; }
PixelFormat TextureGLES::GetFormat() const { return m_pixelFormat; }

// ============================================================================
// SamplerGLES
// ============================================================================

SamplerGLES::SamplerGLES() : m_samplerID(0) {}

SamplerGLES::~SamplerGLES() { Destroy(); }

bool SamplerGLES::Create(const SamplerDescriptor& desc) {
    if (m_samplerID != 0) {
        Destroy();
    }

    glGenSamplers(1, &m_samplerID);
    if (m_samplerID == 0) {
        return false;
    }

    glSamplerParameteri(m_samplerID, GL_TEXTURE_MIN_FILTER, ToGLESFilterMode(desc.minFilter, true));
    glSamplerParameteri(m_samplerID, GL_TEXTURE_MAG_FILTER, ToGLESFilterMode(desc.magFilter, false));
    glSamplerParameteri(m_samplerID, GL_TEXTURE_WRAP_S, ToGLESWrapMode(desc.wrapU, false));
    glSamplerParameteri(m_samplerID, GL_TEXTURE_WRAP_T, ToGLESWrapMode(desc.wrapV, false));
    glSamplerParameteri(m_samplerID, GL_TEXTURE_WRAP_R, ToGLESWrapMode(desc.wrapW, false));

    // 各向异性过滤
    if (desc.maxAnisotropy > 1.0f) {
        GLfloat maxAniso = 1.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
        if (maxAniso > 1.0f) {
            float aniso = std::min(desc.maxAnisotropy, maxAniso);
            glSamplerParameterf(m_samplerID, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
        }
    }

    // 比较模式
    if (desc.compareMode) {
        glSamplerParameteri(m_samplerID, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glSamplerParameteri(m_samplerID, GL_TEXTURE_COMPARE_FUNC,
                            ToGLESCompareFunc(static_cast<CompareFunc>(desc.compareFuncValue)));
    }

    return true;
}

void SamplerGLES::Destroy() {
    if (m_samplerID != 0) {
        glDeleteSamplers(1, &m_samplerID);
        m_samplerID = 0;
    }
}

void SamplerGLES::Bind(uint32_t slot) {
    if (m_samplerID != 0) {
        glBindSampler(slot, m_samplerID);
    }
}

void SamplerGLES::Unbind(uint32_t slot) { glBindSampler(slot, 0); }

} // namespace gles
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGLES
