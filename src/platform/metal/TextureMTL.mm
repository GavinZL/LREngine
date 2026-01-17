/**
 * @file TextureMTL.mm
 * @brief Metal纹理实现
 */

#include "TextureMTL.h"

#ifdef LRENGINE_ENABLE_METAL

#include "TypeConverterMTL.h"
#include "lrengine/core/LRError.h"

namespace lrengine {
namespace render {
namespace mtl {

TextureMTL::TextureMTL(id<MTLDevice> device)
    : m_device(device)
    , m_texture(nil)
    , m_sampler(nil)
    , m_width(0)
    , m_height(0)
    , m_depth(1)
    , m_mipLevels(1)
    , m_type(TextureType::Texture2D)
    , m_format(PixelFormat::RGBA8)
{
}

TextureMTL::~TextureMTL() {
    Destroy();
}

bool TextureMTL::Create(const TextureDescriptor& desc) {
    m_width = desc.width;
    m_height = desc.height;
    m_depth = desc.depth;
    m_type = desc.type;
    m_format = desc.format;
    
    // 计算mipmap层数
    if (desc.mipLevels == 0) {
        uint32_t maxDim = std::max(m_width, m_height);
        m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(maxDim))) + 1;
    } else {
        m_mipLevels = desc.mipLevels;
    }

    // 创建纹理描述符
    MTLTextureDescriptor* texDesc = [[MTLTextureDescriptor alloc] init];
    texDesc.textureType = ToMTLTextureType(desc.type);
    texDesc.pixelFormat = ToMTLPixelFormat(desc.format);
    texDesc.width = desc.width;
    texDesc.height = desc.height;
    texDesc.depth = (desc.type == TextureType::Texture3D) ? desc.depth : 1;
    texDesc.mipmapLevelCount = m_mipLevels;
    texDesc.sampleCount = desc.sampleCount;
    texDesc.arrayLength = (desc.type == TextureType::Texture2DArray) ? desc.depth : 1;
    
    // 设置存储模式和使用标志
    // iOS优先使用Shared模式以获得更好的兼容性
    #if TARGET_OS_IPHONE || TARGET_OS_IOS
        texDesc.storageMode = MTLStorageModeShared;
    #else
        // macOS支持更多存储模式
        texDesc.storageMode = MTLStorageModeManaged;
    #endif
    
    texDesc.usage = MTLTextureUsageShaderRead;
    
    // 深度纹理需要特殊设置
    if (IsDepthFormat(desc.format)) {
        texDesc.usage |= MTLTextureUsageRenderTarget;
        texDesc.storageMode = MTLStorageModePrivate;  // 深度纹理在所有平台都用Private
    }

    m_texture = [m_device newTextureWithDescriptor:texDesc];
    
    if (!m_texture) {
        LR_SET_ERROR(ErrorCode::ResourceCreationFailed, "Failed to create Metal texture");
        return false;
    }

    if (desc.debugName) {
        m_texture.label = [NSString stringWithUTF8String:desc.debugName];
    }

    // 上传初始数据
    if (desc.data) {
        UpdateData(desc.data, 0, nullptr);
    }

    // 生成Mipmap
    if (desc.generateMipmaps && m_mipLevels > 1 && desc.data) {
        GenerateMipmaps();
    }

    // 创建采样器
    if (!CreateSampler(desc.sampler)) {
        return false;
    }

    return true;
}

void TextureMTL::Destroy() {
    m_texture = nil;
    m_sampler = nil;
    m_width = 0;
    m_height = 0;
    m_depth = 1;
    m_mipLevels = 1;
}

void TextureMTL::UpdateData(const void* data, uint32_t mipLevel, const TextureRegion* region) {
    if (!m_texture || !data) {
        LR_SET_ERROR(ErrorCode::InvalidState, "Texture or data is null");
        return;
    }

    uint32_t x = 0, y = 0, z = 0;
    uint32_t width = m_width >> mipLevel;
    uint32_t height = m_height >> mipLevel;
    uint32_t depth = 1;
    
    if (width == 0) width = 1;
    if (height == 0) height = 1;

    if (region) {
        x = region->x;
        y = region->y;
        z = region->z;
        width = region->width;
        height = region->height;
        depth = region->depth;
    }

    uint32_t bytesPerPixel = GetPixelFormatSize(m_format);
    uint32_t bytesPerRow = width * bytesPerPixel;
    uint32_t bytesPerImage = bytesPerRow * height;

    MTLRegion mtlRegion = MTLRegionMake3D(x, y, z, width, height, depth);
    
    [m_texture replaceRegion:mtlRegion
                 mipmapLevel:mipLevel
                       slice:0
                   withBytes:data
                 bytesPerRow:bytesPerRow
               bytesPerImage:bytesPerImage];
}

void TextureMTL::GenerateMipmaps() {
    if (!m_texture || m_mipLevels <= 1) {
        LR_SET_ERROR(ErrorCode::InvalidState, "Texture is null or has only one mipmap level");
        return;
    }

    // 创建命令队列和缓冲区来生成mipmap
    id<MTLCommandQueue> commandQueue = [m_device newCommandQueue];
    id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
    id<MTLBlitCommandEncoder> blitEncoder = [commandBuffer blitCommandEncoder];
    
    [blitEncoder generateMipmapsForTexture:m_texture];
    [blitEncoder endEncoding];
    
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
}

void TextureMTL::Bind(uint32_t slot) {
    LR_UNUSED(slot);
    // Metal中纹理绑定在编码命令时完成
}

void TextureMTL::Unbind(uint32_t slot) {
    LR_UNUSED(slot);
    // Metal中纹理绑定在编码命令时完成
}

ResourceHandle TextureMTL::GetNativeHandle() const {
    return ResourceHandle((__bridge void*)m_texture);
}

uint32_t TextureMTL::GetWidth() const {
    return m_width;
}

uint32_t TextureMTL::GetHeight() const {
    return m_height;
}

uint32_t TextureMTL::GetDepth() const {
    return m_depth;
}

TextureType TextureMTL::GetType() const {
    return m_type;
}

PixelFormat TextureMTL::GetFormat() const {
    return m_format;
}

uint32_t TextureMTL::GetMipLevels() const {
    return m_mipLevels;
}

bool TextureMTL::CreateSampler(const SamplerDescriptor& desc) {
    MTLSamplerDescriptor* samplerDesc = [[MTLSamplerDescriptor alloc] init];
    
    samplerDesc.minFilter = ToMTLSamplerMinMagFilter(desc.minFilter);
    samplerDesc.magFilter = ToMTLSamplerMinMagFilter(desc.magFilter);
    samplerDesc.mipFilter = ToMTLSamplerMipFilter(desc.mipFilter);
    
    samplerDesc.sAddressMode = ToMTLSamplerAddressMode(desc.wrapU);
    samplerDesc.tAddressMode = ToMTLSamplerAddressMode(desc.wrapV);
    samplerDesc.rAddressMode = ToMTLSamplerAddressMode(desc.wrapW);
    
    samplerDesc.lodMinClamp = desc.minLod;
    samplerDesc.lodMaxClamp = desc.maxLod;
    samplerDesc.maxAnisotropy = static_cast<NSUInteger>(desc.maxAnisotropy);
    
    if (desc.compareMode) {
        samplerDesc.compareFunction = ToMTLCompareFunction(static_cast<CompareFunc>(desc.compareFuncValue));
    }

    m_sampler = [m_device newSamplerStateWithDescriptor:samplerDesc];
    
    if (!m_sampler) {
        LR_SET_ERROR(ErrorCode::ResourceCreationFailed, "Failed to create Metal sampler state");
        return false;
    }

    return true;
}

} // namespace mtl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
