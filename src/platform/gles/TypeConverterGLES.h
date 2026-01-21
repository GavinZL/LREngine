/**
 * @file TypeConverterGLES.h
 * @brief OpenGL ES类型转换工具
 */

#pragma once

// 消除废弃警告
#define GL_SILENCE_DEPRECATION

#include "lrengine/core/LRTypes.h"

#ifdef LRENGINE_ENABLE_OPENGLES

// 平台相关的OpenGL ES头文件
#if defined(__ANDROID__)
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <GLES3/gl31.h>
#include <GLES3/gl32.h>
#define LRENGINE_GLES_AVAILABLE 1
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE || TARGET_OS_SIMULATOR
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
#define LRENGINE_GLES_AVAILABLE 1
#else
// macOS不支持OpenGL ES，使用标准OpenGL头文件提供类型定义
#include <OpenGL/gl3.h>
#define LRENGINE_GLES_AVAILABLE 0
#endif
#elif defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#include <GLES3/gl2ext.h>
#define LRENGINE_GLES_AVAILABLE 1
#else
// 其他平台尝试使用标准GL头文件
#include <GL/gl.h>
#define LRENGINE_GLES_AVAILABLE 0
#endif

#if !LRENGINE_GLES_AVAILABLE
#warning                                                                                           \
    "OpenGL ES is not available on this platform. GLES backend will use OpenGL types for compilation but may not work at runtime."
#endif

// OpenGL ES 兼容性定义
// GL_STENCIL_INDEX 在 OpenGL ES 中不存在，但在某些场景需要用到
#ifndef GL_STENCIL_INDEX
#define GL_STENCIL_INDEX 0x1901
#endif

// 前向声明
namespace lrengine {
namespace render {
namespace gles {
class GLESCapabilities;
}
} // namespace render
} // namespace lrengine

namespace lrengine {
namespace render {
namespace gles {

/**
 * @brief 转换缓冲区使用模式到OpenGL ES枚举
 */
inline GLenum ToGLESBufferUsage(BufferUsage usage) {
    switch (usage) {
        case BufferUsage::Static:
            return GL_STATIC_DRAW;
        case BufferUsage::Dynamic:
            return GL_DYNAMIC_DRAW;
        case BufferUsage::Stream:
            return GL_STREAM_DRAW;
        default:
            return GL_STATIC_DRAW;
    }
}

/**
 * @brief 转换缓冲区类型到OpenGL ES目标
 */
inline GLenum ToGLESBufferTarget(BufferType type) {
    switch (type) {
        case BufferType::Vertex:
            return GL_ARRAY_BUFFER;
        case BufferType::Index:
            return GL_ELEMENT_ARRAY_BUFFER;
        case BufferType::Uniform:
            return GL_UNIFORM_BUFFER;
#ifdef GL_SHADER_STORAGE_BUFFER
        case BufferType::Storage:
            return GL_SHADER_STORAGE_BUFFER; // ES 3.1+
#endif
        default:
            return GL_ARRAY_BUFFER;
    }
}

/**
 * @brief 转换着色器阶段到OpenGL ES枚举
 * @note OpenGL ES 3.0不支持Geometry/Tessellation着色器
 */
inline GLenum ToGLESShaderStage(ShaderStage stage) {
    switch (stage) {
        case ShaderStage::Vertex:
            return GL_VERTEX_SHADER;
        case ShaderStage::Fragment:
            return GL_FRAGMENT_SHADER;
#ifdef GL_COMPUTE_SHADER
        case ShaderStage::Compute:
            return GL_COMPUTE_SHADER; // ES 3.1+
#endif
        // ES不支持Geometry和Tessellation着色器
        default:
            return GL_VERTEX_SHADER;
    }
}

/**
 * @brief 转换纹理类型到OpenGL ES目标
 */
inline GLenum ToGLESTextureTarget(TextureType type) {
    switch (type) {
        case TextureType::Texture2D:
            return GL_TEXTURE_2D;
        case TextureType::Texture3D:
            return GL_TEXTURE_3D;
        case TextureType::TextureCube:
            return GL_TEXTURE_CUBE_MAP;
        case TextureType::Texture2DArray:
            return GL_TEXTURE_2D_ARRAY;
#ifdef GL_TEXTURE_2D_MULTISAMPLE
        case TextureType::Texture2DMultisample:
            return GL_TEXTURE_2D_MULTISAMPLE; // ES 3.1+
#endif
        default:
            return GL_TEXTURE_2D;
    }
}

/**
 * @brief 转换像素格式到OpenGL ES内部格式
 * @note 部分格式在ES中支持有限
 */
inline GLenum ToGLESInternalFormat(PixelFormat format) {
    switch (format) {
        // 基本颜色格式
        case PixelFormat::R8:
            return GL_R8;
        case PixelFormat::RG8:
            return GL_RG8;
        case PixelFormat::RGB8:
            return GL_RGB8;
        case PixelFormat::RGBA8:
            return GL_RGBA8;
        // 浮点格式
        case PixelFormat::R16F:
            return GL_R16F;
        case PixelFormat::RG16F:
            return GL_RG16F;
        case PixelFormat::RGB16F:
            return GL_RGB16F;
        case PixelFormat::RGBA16F:
            return GL_RGBA16F;
        case PixelFormat::R32F:
            return GL_R32F;
        case PixelFormat::RG32F:
            return GL_RG32F;
        case PixelFormat::RGB32F:
            return GL_RGB32F;
        case PixelFormat::RGBA32F:
            return GL_RGBA32F;
        case PixelFormat::RGB10A2:
            return GL_RGB10_A2;
        // 深度/模板格式
        case PixelFormat::Depth16:
            return GL_DEPTH_COMPONENT16;
        case PixelFormat::Depth24:
            return GL_DEPTH_COMPONENT24;
        case PixelFormat::Depth32F:
            return GL_DEPTH_COMPONENT32F;
        case PixelFormat::Depth24Stencil8:
            return GL_DEPTH24_STENCIL8;
        case PixelFormat::Depth32FStencil8:
            return GL_DEPTH32F_STENCIL8;
        case PixelFormat::Stencil8:
            return GL_STENCIL_INDEX8;
            // 压缩格式 - ETC2 (ES 3.0原生支持)
#ifdef GL_COMPRESSED_RGB8_ETC2
        case PixelFormat::ETC2_RGB8:
            return GL_COMPRESSED_RGB8_ETC2;
        case PixelFormat::ETC2_RGBA8:
            return GL_COMPRESSED_RGBA8_ETC2_EAC;
#endif
            // ASTC格式 (需要扩展支持)
#ifdef GL_COMPRESSED_RGBA_ASTC_4x4_KHR
        case PixelFormat::ASTC_4x4:
            return GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
        case PixelFormat::ASTC_6x6:
            return GL_COMPRESSED_RGBA_ASTC_6x6_KHR;
        case PixelFormat::ASTC_8x8:
            return GL_COMPRESSED_RGBA_ASTC_8x8_KHR;
#endif
        default:
            return GL_RGBA8;
    }
}

/**
 * @brief 转换像素格式到OpenGL ES格式
 */
inline GLenum ToGLESFormat(PixelFormat format) {
    switch (format) {
        case PixelFormat::R8:
        case PixelFormat::R16F:
        case PixelFormat::R32F:
            return GL_RED;
        case PixelFormat::RG8:
        case PixelFormat::RG16F:
        case PixelFormat::RG32F:
            return GL_RG;
        case PixelFormat::RGB8:
        case PixelFormat::RGB16F:
        case PixelFormat::RGB32F:
            return GL_RGB;
        case PixelFormat::RGBA8:
        case PixelFormat::RGBA16F:
        case PixelFormat::RGBA32F:
        case PixelFormat::RGB10A2:
            return GL_RGBA;
        case PixelFormat::Depth16:
        case PixelFormat::Depth24:
        case PixelFormat::Depth32F:
            return GL_DEPTH_COMPONENT;
        case PixelFormat::Depth24Stencil8:
        case PixelFormat::Depth32FStencil8:
            return GL_DEPTH_STENCIL;
        case PixelFormat::Stencil8:
            return GL_STENCIL_INDEX;
        default:
            return GL_RGBA;
    }
}

/**
 * @brief 转换像素格式到OpenGL ES类型
 */
inline GLenum ToGLESType(PixelFormat format) {
    switch (format) {
        case PixelFormat::R8:
        case PixelFormat::RG8:
        case PixelFormat::RGB8:
        case PixelFormat::RGBA8:
        case PixelFormat::Stencil8:
            return GL_UNSIGNED_BYTE;
        case PixelFormat::R16F:
        case PixelFormat::RG16F:
        case PixelFormat::RGB16F:
        case PixelFormat::RGBA16F:
            return GL_HALF_FLOAT;
        case PixelFormat::R32F:
        case PixelFormat::RG32F:
        case PixelFormat::RGB32F:
        case PixelFormat::RGBA32F:
        case PixelFormat::Depth32F:
            return GL_FLOAT;
        case PixelFormat::RGB10A2:
            return GL_UNSIGNED_INT_2_10_10_10_REV;
        case PixelFormat::Depth16:
            return GL_UNSIGNED_SHORT;
        case PixelFormat::Depth24:
            return GL_UNSIGNED_INT;
        case PixelFormat::Depth24Stencil8:
            return GL_UNSIGNED_INT_24_8;
        case PixelFormat::Depth32FStencil8:
            return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
        default:
            return GL_UNSIGNED_BYTE;
    }
}

/**
 * @brief 转换图元类型到OpenGL ES枚举
 */
inline GLenum ToGLESPrimitiveType(PrimitiveType type) {
    switch (type) {
        case PrimitiveType::Points:
            return GL_POINTS;
        case PrimitiveType::Lines:
            return GL_LINES;
        case PrimitiveType::LineStrip:
            return GL_LINE_STRIP;
        case PrimitiveType::Triangles:
            return GL_TRIANGLES;
        case PrimitiveType::TriangleStrip:
            return GL_TRIANGLE_STRIP;
        case PrimitiveType::TriangleFan:
            return GL_TRIANGLE_FAN;
        default:
            return GL_TRIANGLES;
    }
}

/**
 * @brief 转换索引类型到OpenGL ES枚举
 */
inline GLenum ToGLESIndexType(IndexType type) {
    switch (type) {
        case IndexType::UInt16:
            return GL_UNSIGNED_SHORT;
        case IndexType::UInt32:
            return GL_UNSIGNED_INT;
        default:
            return GL_UNSIGNED_INT;
    }
}

/**
 * @brief 转换比较函数到OpenGL ES枚举
 */
inline GLenum ToGLESCompareFunc(CompareFunc func) {
    switch (func) {
        case CompareFunc::Never:
            return GL_NEVER;
        case CompareFunc::Less:
            return GL_LESS;
        case CompareFunc::Equal:
            return GL_EQUAL;
        case CompareFunc::LessEqual:
            return GL_LEQUAL;
        case CompareFunc::Greater:
            return GL_GREATER;
        case CompareFunc::NotEqual:
            return GL_NOTEQUAL;
        case CompareFunc::GreaterEqual:
            return GL_GEQUAL;
        case CompareFunc::Always:
            return GL_ALWAYS;
        default:
            return GL_LESS;
    }
}

/**
 * @brief 转换混合因子到OpenGL ES枚举
 */
inline GLenum ToGLESBlendFactor(BlendFactor factor) {
    switch (factor) {
        case BlendFactor::Zero:
            return GL_ZERO;
        case BlendFactor::One:
            return GL_ONE;
        case BlendFactor::SrcColor:
            return GL_SRC_COLOR;
        case BlendFactor::OneMinusSrcColor:
            return GL_ONE_MINUS_SRC_COLOR;
        case BlendFactor::DstColor:
            return GL_DST_COLOR;
        case BlendFactor::OneMinusDstColor:
            return GL_ONE_MINUS_DST_COLOR;
        case BlendFactor::SrcAlpha:
            return GL_SRC_ALPHA;
        case BlendFactor::OneMinusSrcAlpha:
            return GL_ONE_MINUS_SRC_ALPHA;
        case BlendFactor::DstAlpha:
            return GL_DST_ALPHA;
        case BlendFactor::OneMinusDstAlpha:
            return GL_ONE_MINUS_DST_ALPHA;
        case BlendFactor::ConstantColor:
            return GL_CONSTANT_COLOR;
        case BlendFactor::OneMinusConstantColor:
            return GL_ONE_MINUS_CONSTANT_COLOR;
        case BlendFactor::ConstantAlpha:
            return GL_CONSTANT_ALPHA;
        case BlendFactor::OneMinusConstantAlpha:
            return GL_ONE_MINUS_CONSTANT_ALPHA;
        case BlendFactor::SrcAlphaSaturate:
            return GL_SRC_ALPHA_SATURATE;
        default:
            return GL_ONE;
    }
}

/**
 * @brief 转换混合操作到OpenGL ES枚举
 */
inline GLenum ToGLESBlendOp(BlendOp op) {
    switch (op) {
        case BlendOp::Add:
            return GL_FUNC_ADD;
        case BlendOp::Subtract:
            return GL_FUNC_SUBTRACT;
        case BlendOp::ReverseSubtract:
            return GL_FUNC_REVERSE_SUBTRACT;
        case BlendOp::Min:
            return GL_MIN;
        case BlendOp::Max:
            return GL_MAX;
        default:
            return GL_FUNC_ADD;
    }
}

/**
 * @brief 转换剔除模式到OpenGL ES枚举
 */
inline GLenum ToGLESCullMode(CullMode mode) {
    switch (mode) {
        case CullMode::Front:
            return GL_FRONT;
        case CullMode::Back:
            return GL_BACK;
        case CullMode::FrontAndBack:
            return GL_FRONT_AND_BACK;
        default:
            return GL_BACK;
    }
}

/**
 * @brief 转换正面朝向到OpenGL ES枚举
 */
inline GLenum ToGLESFrontFace(FrontFace face) {
    switch (face) {
        case FrontFace::CCW:
            return GL_CCW;
        case FrontFace::CW:
            return GL_CW;
        default:
            return GL_CCW;
    }
}

/**
 * @brief 转换模板操作到OpenGL ES枚举
 */
inline GLenum ToGLESStencilOp(StencilOp op) {
    switch (op) {
        case StencilOp::Keep:
            return GL_KEEP;
        case StencilOp::Zero:
            return GL_ZERO;
        case StencilOp::Replace:
            return GL_REPLACE;
        case StencilOp::Increment:
            return GL_INCR;
        case StencilOp::IncrementWrap:
            return GL_INCR_WRAP;
        case StencilOp::Decrement:
            return GL_DECR;
        case StencilOp::DecrementWrap:
            return GL_DECR_WRAP;
        case StencilOp::Invert:
            return GL_INVERT;
        default:
            return GL_KEEP;
    }
}

/**
 * @brief 转换顶点格式到OpenGL ES组件数
 */
inline GLint GetGLESVertexFormatComponents(VertexFormat format) {
    switch (format) {
        case VertexFormat::Float:
        case VertexFormat::Int:
        case VertexFormat::UInt:
            return 1;
        case VertexFormat::Float2:
        case VertexFormat::Int2:
        case VertexFormat::UInt2:
        case VertexFormat::Short2:
        case VertexFormat::UShort2:
            return 2;
        case VertexFormat::Float3:
        case VertexFormat::Int3:
        case VertexFormat::UInt3:
            return 3;
        case VertexFormat::Float4:
        case VertexFormat::Int4:
        case VertexFormat::UInt4:
        case VertexFormat::Short4:
        case VertexFormat::UShort4:
        case VertexFormat::Byte4:
        case VertexFormat::UByte4:
        case VertexFormat::Byte4Norm:
        case VertexFormat::UByte4Norm:
            return 4;
        default:
            return 4;
    }
}

/**
 * @brief 转换顶点格式到OpenGL ES类型
 */
inline GLenum GetGLESVertexFormatType(VertexFormat format) {
    switch (format) {
        case VertexFormat::Float:
        case VertexFormat::Float2:
        case VertexFormat::Float3:
        case VertexFormat::Float4:
            return GL_FLOAT;
        case VertexFormat::Int:
        case VertexFormat::Int2:
        case VertexFormat::Int3:
        case VertexFormat::Int4:
            return GL_INT;
        case VertexFormat::UInt:
        case VertexFormat::UInt2:
        case VertexFormat::UInt3:
        case VertexFormat::UInt4:
            return GL_UNSIGNED_INT;
        case VertexFormat::Short2:
        case VertexFormat::Short4:
            return GL_SHORT;
        case VertexFormat::UShort2:
        case VertexFormat::UShort4:
            return GL_UNSIGNED_SHORT;
        case VertexFormat::Byte4:
        case VertexFormat::Byte4Norm:
            return GL_BYTE;
        case VertexFormat::UByte4:
        case VertexFormat::UByte4Norm:
            return GL_UNSIGNED_BYTE;
        default:
            return GL_FLOAT;
    }
}

/**
 * @brief 检查顶点格式是否需要归一化
 */
inline GLboolean IsGLESVertexFormatNormalized(VertexFormat format) {
    switch (format) {
        case VertexFormat::Byte4Norm:
        case VertexFormat::UByte4Norm:
            return GL_TRUE;
        default:
            return GL_FALSE;
    }
}

/**
 * @brief 转换内存访问模式到OpenGL ES标志
 */
inline GLbitfield ToGLESMapAccess(MemoryAccess access) {
    switch (access) {
        case MemoryAccess::ReadOnly:
            return GL_MAP_READ_BIT;
        case MemoryAccess::WriteOnly:
            return GL_MAP_WRITE_BIT;
        case MemoryAccess::ReadWrite:
            return GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
        default:
            return GL_MAP_WRITE_BIT;
    }
}

/**
 * @brief 转换过滤模式到OpenGL ES枚举
 */
inline GLenum ToGLESFilterMode(FilterMode mode, bool mipmap = false) {
    switch (mode) {
        case FilterMode::Nearest:
            return mipmap ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
        case FilterMode::Linear:
            return mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
        case FilterMode::NearestMipmapNearest:
            return GL_NEAREST_MIPMAP_NEAREST;
        case FilterMode::LinearMipmapNearest:
            return GL_LINEAR_MIPMAP_NEAREST;
        case FilterMode::NearestMipmapLinear:
            return GL_NEAREST_MIPMAP_LINEAR;
        case FilterMode::LinearMipmapLinear:
            return GL_LINEAR_MIPMAP_LINEAR;
        default:
            return GL_LINEAR;
    }
}

/**
 * @brief 转换包裹模式到OpenGL ES枚举
 * @note GL_CLAMP_TO_BORDER在ES中需要扩展支持
 */
inline GLenum ToGLESWrapMode(WrapMode mode, bool hasBorderClampExt = false) {
    switch (mode) {
        case WrapMode::Repeat:
            return GL_REPEAT;
        case WrapMode::MirroredRepeat:
            return GL_MIRRORED_REPEAT;
        case WrapMode::ClampToEdge:
            return GL_CLAMP_TO_EDGE;
        case WrapMode::ClampToBorder:
            // ES需要GL_EXT_texture_border_clamp扩展
#ifdef GL_CLAMP_TO_BORDER_EXT
            return hasBorderClampExt ? GL_CLAMP_TO_BORDER_EXT : GL_CLAMP_TO_EDGE;
#else
            return GL_CLAMP_TO_EDGE; // 回退到ClampToEdge
#endif
        default:
            return GL_REPEAT;
    }
}

} // namespace gles
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGLES
