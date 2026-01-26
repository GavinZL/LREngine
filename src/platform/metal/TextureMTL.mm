/**
 * @file TextureMTL.mm
 * @brief Metal纹理实现
 */

#include "TextureMTL.h"

#ifdef LRENGINE_ENABLE_METAL

#include "TypeConverterMTL.h"
#include "lrengine/core/LRError.h"
#include "lrengine/utils/ImageBuffer.h"
#include <CoreVideo/CoreVideo.h>

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
    
    // 设置纹理使用标志
    texDesc.usage = MTLTextureUsageShaderRead;
    
    // 判断是否用作渲染目标
    bool isRenderTarget = (desc.data == nullptr) || IsDepthFormat(desc.format);
    if (isRenderTarget) {
        texDesc.usage |= MTLTextureUsageRenderTarget;
    }
    
    // 设置存储模式：根据平台、纹理类型和用途选择最优模式
    if (IsDepthFormat(desc.format)) {
        // 深度/模板纹理：所有平台都使用Private模式
        texDesc.storageMode = MTLStorageModePrivate;
    } else if (desc.data == nullptr) {
        // 离屏渲染纹理（无初始数据）
        #if TARGET_OS_IPHONE || TARGET_OS_IOS
            // iOS: 如果仅用于RenderTarget且不需CPU访问，使用Private模式更高效
            // 但为了兼容性（支持后续可能的CPU读取），默认使用Shared
            texDesc.storageMode = MTLStorageModeShared;
        #else
            // macOS: 离屏渲染纹理使用Private模式（GPU专用，最佳性能）
            texDesc.storageMode = MTLStorageModePrivate;
        #endif
    } else {
        // 有初始数据的纹理：需要CPU写入
        #if TARGET_OS_IPHONE || TARGET_OS_IOS
            texDesc.storageMode = MTLStorageModeShared;
        #else
            texDesc.storageMode = MTLStorageModeManaged;  // macOS支持CPU写入后同步到GPU
        #endif
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

bool TextureMTL::ReadbackTo(utils::ImageBuffer* buffer, uint32_t mipLevel) {
    if (!m_texture || !buffer) {
        LR_SET_ERROR(ErrorCode::InvalidState, "Texture or buffer is null");
        return false;
    }

    // 验证缓冲区格式和尺寸是否匹配
    const auto& bufferDesc = buffer->GetImageDesc();
    uint32_t mipWidth = m_width >> mipLevel;
    uint32_t mipHeight = m_height >> mipLevel;
    if (mipWidth == 0) mipWidth = 1;
    if (mipHeight == 0) mipHeight = 1;

    if (bufferDesc.width != mipWidth || bufferDesc.height != mipHeight) {
        LR_SET_ERROR(ErrorCode::InvalidState, "Buffer size mismatch");
        return false;
    }

    // 锁定缓冲区以获取 CPU 可访问的内存
    if (!buffer->Lock(false)) {  // false = 写入模式
        LR_SET_ERROR(ErrorCode::InvalidState, "Failed to lock buffer");
        return false;
    }

    // 根据缓冲区类型选择不同的回读策略
    bool success = false;
    
    if (buffer->GetBufferType() == utils::ImageBuffer::BufferType::CVPixelBuffer) {
        // iOS/macOS: 使用 CVPixelBuffer 的零拷贝机制
        #ifdef __APPLE__
        CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)buffer->GetNativeBuffer();
        if (pixelBuffer) {
            // 创建临时纹理从 CVPixelBuffer
            // 然后使用 blit 命令复制
            // TODO: 完整实现需要 CVMetalTextureCache
            success = false;  // 暂时返回 false，待完整实现
        }
        #endif
    } else {
        // 通用路径：使用 getBytes 同步读取
        // 注意：这要求纹理的 storageMode 支持 CPU 访问
        uint32_t bytesPerPixel = GetPixelFormatSize(m_format);
        uint32_t bytesPerRow = mipWidth * bytesPerPixel;
        
        MTLRegion region = MTLRegionMake2D(0, 0, mipWidth, mipHeight);
        
        // 对于单平面格式，直接读取
        if (bufferDesc.planes.size() > 0 && bufferDesc.planes[0].data) {
            void* destData = const_cast<void*>(bufferDesc.planes[0].data);
            uint32_t destStride = bufferDesc.planes[0].stride;
            
            if (destStride == 0 || destStride == bytesPerRow) {
                // 紧密排列，可以一次性读取
                [m_texture getBytes:destData
                        bytesPerRow:bytesPerRow
                         fromRegion:region
                        mipmapLevel:mipLevel];
                success = true;
            } else {
                // 需要按行复制
                std::vector<uint8_t> tempBuffer(bytesPerRow * mipHeight);
                [m_texture getBytes:tempBuffer.data()
                        bytesPerRow:bytesPerRow
                         fromRegion:region
                        mipmapLevel:mipLevel];
                
                // 按行复制到目标缓冲区
                uint8_t* src = tempBuffer.data();
                uint8_t* dst = static_cast<uint8_t*>(destData);
                for (uint32_t y = 0; y < mipHeight; ++y) {
                    std::memcpy(dst, src, bytesPerRow);
                    src += bytesPerRow;
                    dst += destStride;
                }
                success = true;
            }
        }
    }

    buffer->Unlock();
    return success;
}

} // namespace mtl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
