/**
 * @file TypeConverterMTL.h
 * @brief Metal类型转换工具
 */

#pragma once

#include "lrengine/core/LRTypes.h"

#ifdef LRENGINE_ENABLE_METAL

#import <Metal/Metal.h>

namespace lrengine {
namespace render {
namespace mtl {

/**
 * @brief 转换缓冲区使用模式到Metal存储模式
 */
inline MTLResourceOptions ToMTLResourceOptions(BufferUsage usage) {
    switch (usage) {
        case BufferUsage::Static:
            return MTLResourceStorageModeShared;
        case BufferUsage::Dynamic:
            return MTLResourceStorageModeShared | MTLResourceCPUCacheModeWriteCombined;
        case BufferUsage::Stream:
            return MTLResourceStorageModeShared | MTLResourceCPUCacheModeWriteCombined;
        default:
            return MTLResourceStorageModeShared;
    }
}

/**
 * @brief 转换图元类型到Metal图元类型
 */
inline MTLPrimitiveType ToMTLPrimitiveType(PrimitiveType type) {
    switch (type) {
        case PrimitiveType::Points:        return MTLPrimitiveTypePoint;
        case PrimitiveType::Lines:         return MTLPrimitiveTypeLine;
        case PrimitiveType::LineStrip:     return MTLPrimitiveTypeLineStrip;
        case PrimitiveType::Triangles:     return MTLPrimitiveTypeTriangle;
        case PrimitiveType::TriangleStrip: return MTLPrimitiveTypeTriangleStrip;
        default:                           return MTLPrimitiveTypeTriangle;
    }
}

/**
 * @brief 转换索引类型到Metal索引类型
 */
inline MTLIndexType ToMTLIndexType(IndexType type) {
    switch (type) {
        case IndexType::UInt16: return MTLIndexTypeUInt16;
        case IndexType::UInt32: return MTLIndexTypeUInt32;
        default:                return MTLIndexTypeUInt32;
    }
}

/**
 * @brief 获取索引类型字节大小
 */
inline size_t GetIndexTypeSize(IndexType type) {
    switch (type) {
        case IndexType::UInt16: return 2;
        case IndexType::UInt32: return 4;
        default:                return 4;
    }
}

/**
 * @brief 转换像素格式到Metal像素格式
 * 
 * 注意：不同平台支持的格式有所不同
 * - macOS支持BC压缩格式（桌面GPU）
 * - iOS支持ASTC和PVRTC压缩格式（移动GPU）
 * - 部分深度/模板格式在iOS上有限制
 */
inline MTLPixelFormat ToMTLPixelFormat(PixelFormat format) {
    switch (format) {
        case PixelFormat::R8:               return MTLPixelFormatR8Unorm;
        case PixelFormat::RG8:              return MTLPixelFormatRG8Unorm;
        case PixelFormat::RGBA8:            return MTLPixelFormatRGBA8Unorm;
        case PixelFormat::R16F:             return MTLPixelFormatR16Float;
        case PixelFormat::RG16F:            return MTLPixelFormatRG16Float;
        case PixelFormat::RGBA16F:          return MTLPixelFormatRGBA16Float;
        case PixelFormat::R32F:             return MTLPixelFormatR32Float;
        case PixelFormat::RG32F:            return MTLPixelFormatRG32Float;
        case PixelFormat::RGBA32F:          return MTLPixelFormatRGBA32Float;
        case PixelFormat::RGB10A2:          return MTLPixelFormatRGB10A2Unorm;
        
        // 深度格式
        case PixelFormat::Depth16:          return MTLPixelFormatDepth16Unorm;
        case PixelFormat::Depth32F:         return MTLPixelFormatDepth32Float;
        
        // 深度模板组合格式 - 平台特定处理
        case PixelFormat::Depth24Stencil8:
            #if TARGET_OS_IPHONE || TARGET_OS_IOS
                // iOS不支持Depth24Unorm_Stencil8，回退到Depth32Float_Stencil8
                return MTLPixelFormatDepth32Float_Stencil8;
            #else
                return MTLPixelFormatDepth24Unorm_Stencil8;
            #endif
            
        case PixelFormat::Depth32FStencil8: return MTLPixelFormatDepth32Float_Stencil8;
        case PixelFormat::Stencil8:         return MTLPixelFormatStencil8;
        
        // BC压缩格式（仅macOS支持）
        #if !TARGET_OS_IPHONE && !TARGET_OS_IOS
        case PixelFormat::BC1:              return MTLPixelFormatBC1_RGBA;
        case PixelFormat::BC2:              return MTLPixelFormatBC2_RGBA;
        case PixelFormat::BC3:              return MTLPixelFormatBC3_RGBA;
        case PixelFormat::BC4:              return MTLPixelFormatBC4_RUnorm;
        case PixelFormat::BC5:              return MTLPixelFormatBC5_RGUnorm;
        case PixelFormat::BC6H:             return MTLPixelFormatBC6H_RGBFloat;
        case PixelFormat::BC7:              return MTLPixelFormatBC7_RGBAUnorm;
        #endif
        
        // ASTC压缩格式（iOS和较新的macOS支持）
        case PixelFormat::ASTC_4x4:         return MTLPixelFormatASTC_4x4_LDR;
        case PixelFormat::ASTC_6x6:         return MTLPixelFormatASTC_6x6_LDR;
        case PixelFormat::ASTC_8x8:         return MTLPixelFormatASTC_8x8_LDR;
        
        // ETC2压缩格式（iOS支持，macOS不支持）
        #if TARGET_OS_IPHONE || TARGET_OS_IOS
        case PixelFormat::ETC2_RGB8:        return MTLPixelFormatETC2_RGB8;
        case PixelFormat::ETC2_RGBA8:       return MTLPixelFormatEAC_RGBA8;
        #endif
        
        default:                            return MTLPixelFormatRGBA8Unorm;
    }
}

/**
 * @brief 转换纹理类型到Metal纹理类型
 */
inline MTLTextureType ToMTLTextureType(TextureType type) {
    switch (type) {
        case TextureType::Texture2D:            return MTLTextureType2D;
        case TextureType::Texture3D:            return MTLTextureType3D;
        case TextureType::TextureCube:          return MTLTextureTypeCube;
        case TextureType::Texture2DArray:       return MTLTextureType2DArray;
        case TextureType::Texture2DMultisample: return MTLTextureType2DMultisample;
        default:                                return MTLTextureType2D;
    }
}

/**
 * @brief 转换顶点格式到Metal顶点格式
 */
inline MTLVertexFormat ToMTLVertexFormat(VertexFormat format) {
    switch (format) {
        case VertexFormat::Float:      return MTLVertexFormatFloat;
        case VertexFormat::Float2:     return MTLVertexFormatFloat2;
        case VertexFormat::Float3:     return MTLVertexFormatFloat3;
        case VertexFormat::Float4:     return MTLVertexFormatFloat4;
        case VertexFormat::Int:        return MTLVertexFormatInt;
        case VertexFormat::Int2:       return MTLVertexFormatInt2;
        case VertexFormat::Int3:       return MTLVertexFormatInt3;
        case VertexFormat::Int4:       return MTLVertexFormatInt4;
        case VertexFormat::UInt:       return MTLVertexFormatUInt;
        case VertexFormat::UInt2:      return MTLVertexFormatUInt2;
        case VertexFormat::UInt3:      return MTLVertexFormatUInt3;
        case VertexFormat::UInt4:      return MTLVertexFormatUInt4;
        case VertexFormat::Short2:     return MTLVertexFormatShort2;
        case VertexFormat::Short4:     return MTLVertexFormatShort4;
        case VertexFormat::UShort2:    return MTLVertexFormatUShort2;
        case VertexFormat::UShort4:    return MTLVertexFormatUShort4;
        case VertexFormat::Byte4:      return MTLVertexFormatChar4;
        case VertexFormat::UByte4:     return MTLVertexFormatUChar4;
        case VertexFormat::Byte4Norm:  return MTLVertexFormatChar4Normalized;
        case VertexFormat::UByte4Norm: return MTLVertexFormatUChar4Normalized;
        default:                       return MTLVertexFormatFloat3;
    }
}

/**
 * @brief 转换采样器地址模式
 */
inline MTLSamplerAddressMode ToMTLSamplerAddressMode(WrapMode mode) {
    switch (mode) {
        case WrapMode::Repeat:         return MTLSamplerAddressModeRepeat;
        case WrapMode::ClampToEdge:    return MTLSamplerAddressModeClampToEdge;
        case WrapMode::MirroredRepeat: return MTLSamplerAddressModeMirrorRepeat;
        case WrapMode::ClampToBorder:
            // ClampToBorder需要iOS 14.0+，对于iOS 13.0降级为ClampToEdge
            #if TARGET_OS_IPHONE
                if (@available(iOS 14.0, *)) {
                    return MTLSamplerAddressModeClampToBorderColor;
                } else {
                    return MTLSamplerAddressModeClampToEdge;
                }
            #else
                return MTLSamplerAddressModeClampToBorderColor;
            #endif
        default:                       return MTLSamplerAddressModeRepeat;
    }
}

/**
 * @brief 转换采样器过滤模式
 */
inline MTLSamplerMinMagFilter ToMTLSamplerMinMagFilter(FilterMode mode) {
    switch (mode) {
        case FilterMode::Nearest:
        case FilterMode::NearestMipmapNearest:
        case FilterMode::NearestMipmapLinear:
            return MTLSamplerMinMagFilterNearest;
        default:
            return MTLSamplerMinMagFilterLinear;
    }
}

/**
 * @brief 转换Mipmap过滤模式
 */
inline MTLSamplerMipFilter ToMTLSamplerMipFilter(FilterMode mode) {
    switch (mode) {
        case FilterMode::Nearest:
        case FilterMode::Linear:
            return MTLSamplerMipFilterNotMipmapped;
        case FilterMode::NearestMipmapNearest:
        case FilterMode::LinearMipmapNearest:
            return MTLSamplerMipFilterNearest;
        case FilterMode::NearestMipmapLinear:
        case FilterMode::LinearMipmapLinear:
            return MTLSamplerMipFilterLinear;
        default:
            return MTLSamplerMipFilterLinear;
    }
}

/**
 * @brief 转换比较函数
 */
inline MTLCompareFunction ToMTLCompareFunction(CompareFunc func) {
    switch (func) {
        case CompareFunc::Never:        return MTLCompareFunctionNever;
        case CompareFunc::Less:         return MTLCompareFunctionLess;
        case CompareFunc::Equal:        return MTLCompareFunctionEqual;
        case CompareFunc::LessEqual:    return MTLCompareFunctionLessEqual;
        case CompareFunc::Greater:      return MTLCompareFunctionGreater;
        case CompareFunc::NotEqual:     return MTLCompareFunctionNotEqual;
        case CompareFunc::GreaterEqual: return MTLCompareFunctionGreaterEqual;
        case CompareFunc::Always:       return MTLCompareFunctionAlways;
        default:                        return MTLCompareFunctionLess;
    }
}

/**
 * @brief 转换模板操作
 */
inline MTLStencilOperation ToMTLStencilOperation(StencilOp op) {
    switch (op) {
        case StencilOp::Keep:          return MTLStencilOperationKeep;
        case StencilOp::Zero:          return MTLStencilOperationZero;
        case StencilOp::Replace:       return MTLStencilOperationReplace;
        case StencilOp::Increment:     return MTLStencilOperationIncrementClamp;
        case StencilOp::IncrementWrap: return MTLStencilOperationIncrementWrap;
        case StencilOp::Decrement:     return MTLStencilOperationDecrementClamp;
        case StencilOp::DecrementWrap: return MTLStencilOperationDecrementWrap;
        case StencilOp::Invert:        return MTLStencilOperationInvert;
        default:                       return MTLStencilOperationKeep;
    }
}

/**
 * @brief 转换混合因子
 */
inline MTLBlendFactor ToMTLBlendFactor(BlendFactor factor) {
    switch (factor) {
        case BlendFactor::Zero:                  return MTLBlendFactorZero;
        case BlendFactor::One:                   return MTLBlendFactorOne;
        case BlendFactor::SrcColor:              return MTLBlendFactorSourceColor;
        case BlendFactor::OneMinusSrcColor:      return MTLBlendFactorOneMinusSourceColor;
        case BlendFactor::DstColor:              return MTLBlendFactorDestinationColor;
        case BlendFactor::OneMinusDstColor:      return MTLBlendFactorOneMinusDestinationColor;
        case BlendFactor::SrcAlpha:              return MTLBlendFactorSourceAlpha;
        case BlendFactor::OneMinusSrcAlpha:      return MTLBlendFactorOneMinusSourceAlpha;
        case BlendFactor::DstAlpha:              return MTLBlendFactorDestinationAlpha;
        case BlendFactor::OneMinusDstAlpha:      return MTLBlendFactorOneMinusDestinationAlpha;
        case BlendFactor::ConstantColor:         return MTLBlendFactorBlendColor;
        case BlendFactor::OneMinusConstantColor: return MTLBlendFactorOneMinusBlendColor;
        case BlendFactor::ConstantAlpha:         return MTLBlendFactorBlendAlpha;
        case BlendFactor::OneMinusConstantAlpha: return MTLBlendFactorOneMinusBlendAlpha;
        case BlendFactor::SrcAlphaSaturate:      return MTLBlendFactorSourceAlphaSaturated;
        default:                                 return MTLBlendFactorOne;
    }
}

/**
 * @brief 转换混合操作
 */
inline MTLBlendOperation ToMTLBlendOperation(BlendOp op) {
    switch (op) {
        case BlendOp::Add:             return MTLBlendOperationAdd;
        case BlendOp::Subtract:        return MTLBlendOperationSubtract;
        case BlendOp::ReverseSubtract: return MTLBlendOperationReverseSubtract;
        case BlendOp::Min:             return MTLBlendOperationMin;
        case BlendOp::Max:             return MTLBlendOperationMax;
        default:                       return MTLBlendOperationAdd;
    }
}

/**
 * @brief 转换剔除模式
 */
inline MTLCullMode ToMTLCullMode(CullMode mode) {
    switch (mode) {
        case CullMode::None:  return MTLCullModeNone;
        case CullMode::Front: return MTLCullModeFront;
        case CullMode::Back:  return MTLCullModeBack;
        default:              return MTLCullModeBack;
    }
}

/**
 * @brief 转换正面朝向
 */
inline MTLWinding ToMTLWinding(FrontFace face) {
    switch (face) {
        case FrontFace::CCW: return MTLWindingCounterClockwise;
        case FrontFace::CW:  return MTLWindingClockwise;
        default:             return MTLWindingCounterClockwise;
    }
}

/**
 * @brief 转换填充模式
 */
inline MTLTriangleFillMode ToMTLTriangleFillMode(FillMode mode) {
    switch (mode) {
        case FillMode::Solid:     return MTLTriangleFillModeFill;
        case FillMode::Wireframe: return MTLTriangleFillModeLines;
        default:                  return MTLTriangleFillModeFill;
    }
}

} // namespace mtl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
