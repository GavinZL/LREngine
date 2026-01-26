/**
 * @file TextureMTL.h
 * @brief Metal纹理实现
 */

#pragma once

#include "platform/interface/ITextureImpl.h"

#ifdef LRENGINE_ENABLE_METAL

#import <Metal/Metal.h>

namespace lrengine {
namespace render {
namespace mtl {

/**
 * @brief Metal纹理实现
 */
class TextureMTL : public ITextureImpl {
public:
    TextureMTL(id<MTLDevice> device);
    ~TextureMTL() override;

    // ITextureImpl接口
    bool Create(const TextureDescriptor& desc) override;
    void Destroy() override;
    void UpdateData(const void* data,
                    uint32_t mipLevel           = 0,
                    const TextureRegion* region = nullptr) override;
    void GenerateMipmaps() override;
    void Bind(uint32_t slot) override;
    void Unbind(uint32_t slot) override;
    ResourceHandle GetNativeHandle() const override;
    uint32_t GetWidth() const override;
    uint32_t GetHeight() const override;
    uint32_t GetDepth() const override;
    TextureType GetType() const override;
    PixelFormat GetFormat() const override;
    uint32_t GetMipLevels() const override;

    // Metal特有方法
    id<MTLTexture> GetTexture() const { return m_texture; }
    id<MTLSamplerState> GetSampler() const { return m_sampler; }

    // 阶段 4 新增：Readback 接口
    bool ReadbackTo(utils::ImageBuffer* buffer, uint32_t mipLevel = 0) override;

private:
    bool CreateSampler(const SamplerDescriptor& desc);

    id<MTLDevice> m_device;
    id<MTLTexture> m_texture;
    id<MTLSamplerState> m_sampler;

    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_depth;
    uint32_t m_mipLevels;
    TextureType m_type;
    PixelFormat m_format;
};

} // namespace mtl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
