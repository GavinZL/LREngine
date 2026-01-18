/**
 * @file TextureGLES.h
 * @brief OpenGL ES纹理实现
 */

#pragma once

#include "platform/interface/ITextureImpl.h"
#include "TypeConverterGLES.h"

#ifdef LRENGINE_ENABLE_OPENGLES

namespace lrengine {
namespace render {
namespace gles {

/**
 * @brief OpenGL ES纹理实现
 */
class TextureGLES : public ITextureImpl {
public:
    TextureGLES();
    ~TextureGLES() override;

    // ITextureImpl接口
    bool Create(const TextureDescriptor& desc) override;
    void Destroy() override;
    void UpdateData(const void* data, uint32_t mipLevel, const TextureRegion* region) override;
    void GenerateMipmaps() override;
    void Bind(uint32_t slot) override;
    void Unbind(uint32_t slot) override;
    ResourceHandle GetNativeHandle() const override;
    uint32_t GetWidth() const override;
    uint32_t GetHeight() const override;
    uint32_t GetDepth() const override;
    TextureType GetType() const override;
    PixelFormat GetFormat() const override;
    uint32_t GetMipLevels() const override { return m_mipLevels; }

    // OpenGL ES特有方法
    GLuint GetTextureID() const { return m_textureID; }
    GLenum GetTarget() const { return m_target; }

private:
    void SetSamplerParameters(const TextureDescriptor& desc);

    GLuint m_textureID;
    GLenum m_target;
    GLenum m_internalFormat;
    GLenum m_format;
    GLenum m_dataType;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_depth;
    uint32_t m_mipLevels;
    TextureType m_type;
    PixelFormat m_pixelFormat;
};

/**
 * @brief OpenGL ES采样器实现
 */
class SamplerGLES {
public:
    SamplerGLES();
    ~SamplerGLES();

    bool Create(const SamplerDescriptor& desc);
    void Destroy();
    void Bind(uint32_t slot);
    void Unbind(uint32_t slot);
    
    GLuint GetSamplerID() const { return m_samplerID; }

private:
    GLuint m_samplerID;
};

} // namespace gles
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGLES
