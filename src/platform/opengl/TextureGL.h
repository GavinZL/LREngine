/**
 * @file TextureGL.h
 * @brief OpenGL纹理实现
 */

#pragma once

#include "platform/interface/ITextureImpl.h"
#include "TypeConverterGL.h"

#ifdef LRENGINE_ENABLE_OPENGL

namespace lrengine {
namespace render {
namespace gl {

/**
 * @brief OpenGL纹理实现
 */
class TextureGL : public ITextureImpl {
public:
    TextureGL();
    ~TextureGL() override;

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

    // OpenGL特有方法
    GLuint GetTextureID() const { return m_textureID; }
    GLenum GetTarget() const { return m_target; }
    uint32_t GetMipLevels() const override { return m_mipLevels; }

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
 * @brief OpenGL采样器实现
 */
class SamplerGL {
public:
    SamplerGL();
    ~SamplerGL();

    bool Create(const SamplerDescriptor& desc);
    void Destroy();
    void Bind(uint32_t slot);
    void Unbind(uint32_t slot);
    
    GLuint GetSamplerID() const { return m_samplerID; }

private:
    GLuint m_samplerID;
};

} // namespace gl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGL
