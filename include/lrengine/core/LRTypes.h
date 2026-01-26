/**
 * @file LRTypes.h
 * @brief LREngine核心类型定义 - 枚举和描述符
 */

#pragma once

#include "LRDefines.h"
#include "../math/MathFwd.hpp"

#include <cstdint>
#include <vector>
#include <string>

namespace lrengine {
namespace render {

// 前向声明
class LRShader;

// =============================================================================
// 后端和基础枚举
// =============================================================================

/**
 * @brief 渲染后端API类型
 */
enum class Backend : uint8_t {
    OpenGL,      // OpenGL 3.3+ Core Profile
    OpenGLES,    // OpenGL ES 3.0+
    Metal,       // Metal 2.0+
    Vulkan,      // Vulkan 1.1+
    DirectX12,   // DirectX 12
    Unknown
};

/**
 * @brief 资源类型
 */
enum class ResourceType : uint8_t {
    Buffer,
    VertexBuffer,
    IndexBuffer,
    UniformBuffer,
    Texture,
    Sampler,
    FrameBuffer,
    Shader,
    PipelineState,
    RenderPass,
    Fence,
    Unknown
};

// =============================================================================
// 缓冲区相关
// =============================================================================

/**
 * @brief 缓冲区使用模式
 */
enum class BufferUsage : uint8_t {
    Static,   // 静态数据，很少更新
    Dynamic,  // 动态数据，偶尔更新
    Stream    // 流式数据，每帧更新
};

/**
 * @brief 缓冲区类型
 */
enum class BufferType : uint8_t {
    Vertex,   // 顶点缓冲
    Index,    // 索引缓冲
    Uniform,  // 统一缓冲
    Storage   // 存储缓冲
};

/**
 * @brief 内存访问模式
 */
enum class MemoryAccess : uint8_t {
    ReadOnly,
    WriteOnly,
    ReadWrite
};

/**
 * @brief 索引类型
 */
enum class IndexType : uint8_t {
    UInt16,
    UInt32
};

/**
 * @brief 缓冲区描述符
 */
struct BufferDescriptor {
    size_t size = 0;                           // 缓冲区大小（字节）
    BufferUsage usage = BufferUsage::Static;   // 使用模式
    BufferType type = BufferType::Vertex;      // 缓冲区类型
    const void* data = nullptr;                // 初始化数据指针
    uint32_t stride = 0;                       // 顶点步长（仅顶点缓冲）
    IndexType indexType = IndexType::UInt32;   // 索引类型（仅索引缓冲）
    const char* debugName = nullptr;           // 调试名称
};

// =============================================================================
// 顶点格式相关
// =============================================================================

/**
 * @brief 顶点属性格式
 */
enum class VertexFormat : uint8_t {
    Float,      // float
    Float2,     // vec2
    Float3,     // vec3
    Float4,     // vec4
    Int,        // int
    Int2,       // ivec2
    Int3,       // ivec3
    Int4,       // ivec4
    UInt,       // uint
    UInt2,      // uvec2
    UInt3,      // uvec3
    UInt4,      // uvec4
    Short2,     // short[2]
    Short4,     // short[4]
    UShort2,    // ushort[2]
    UShort4,    // ushort[4]
    Byte4,      // byte[4]
    UByte4,     // ubyte[4]
    Byte4Norm,  // byte[4] normalized
    UByte4Norm  // ubyte[4] normalized
};

/**
 * @brief 顶点属性描述
 */
struct VertexAttribute {
    uint32_t location = 0;                     // 属性位置（着色器中的location）
    VertexFormat format = VertexFormat::Float3;// 数据格式
    uint32_t offset = 0;                       // 字节偏移
    uint32_t stride = 0;                       // 步长（0表示紧密排列）
    bool normalized = false;                   // 是否归一化
};

/**
 * @brief 顶点布局描述符
 */
struct VertexLayoutDescriptor {
    std::vector<VertexAttribute> attributes;
    uint32_t stride = 0;  // 总步长
};

// =============================================================================
// 着色器相关
// =============================================================================

/**
 * @brief 着色器阶段
 */
enum class ShaderStage : uint8_t {
    Vertex   = 0x01,
    Fragment = 0x02,
    Geometry = 0x04,
    Compute  = 0x08,
    TessControl = 0x10,
    TessEval = 0x20
};

/**
 * @brief 着色器源码语言
 */
enum class ShaderLanguage : uint8_t {
    GLSL,    // OpenGL着色器语言
    SPIRV,   // SPIR-V字节码
    MSL,     // Metal着色器语言
    HLSL     // DirectX着色器语言
};

/**
 * @brief 着色器描述符
 */
struct ShaderDescriptor {
    ShaderStage stage = ShaderStage::Vertex;
    ShaderLanguage language = ShaderLanguage::GLSL;
    const char* source = nullptr;              // 源码字符串
    size_t sourceLength = 0;                   // 源码长度（0表示以null结尾）
    const char* entryPoint = "main";           // 入口函数名
    const char* debugName = nullptr;           // 调试名称
};

// =============================================================================
// 纹理相关
// =============================================================================

/**
 * @brief 纹理类型
 */
enum class TextureType : uint8_t {
    Texture2D,
    Texture3D,
    TextureCube,
    Texture2DArray,
    Texture2DMultisample
};

/**
 * @brief 像素格式
 */
enum class PixelFormat : uint8_t {
    // 颜色格式
    R8,
    RG8,
    RGB8,
    RGBA8,
    R16F,
    RG16F,
    RGB16F,
    RGBA16F,
    R32F,
    RG32F,
    RGB32F,
    RGBA32F,
    RGB10A2,
    // 深度/模板格式
    Depth16,
    Depth24,
    Depth32F,
    Depth24Stencil8,
    Depth32FStencil8,
    Stencil8,
    // 压缩格式
    BC1,  // DXT1
    BC2,  // DXT3
    BC3,  // DXT5
    BC4,
    BC5,
    BC6H,
    BC7,
    // ASTC
    ASTC_4x4,
    ASTC_6x6,
    ASTC_8x8,
    // ETC
    ETC2_RGB8,
    ETC2_RGBA8
};

/**
 * @brief 纹理过滤模式
 */
enum class FilterMode : uint8_t {
    Nearest,
    Linear,
    NearestMipmapNearest,
    LinearMipmapNearest,
    NearestMipmapLinear,
    LinearMipmapLinear
};

/**
 * @brief 纹理包裹模式
 */
enum class WrapMode : uint8_t {
    Repeat,
    ClampToEdge,
    MirroredRepeat,
    ClampToBorder
};

// =============================================================================
// 采样器相关（移到纹理描述符之前）
// =============================================================================

/**
 * @brief 采样器描述符
 */
struct SamplerDescriptor {
    FilterMode minFilter = FilterMode::Linear;
    FilterMode magFilter = FilterMode::Linear;
    FilterMode mipFilter = FilterMode::LinearMipmapLinear;
    WrapMode wrapU = WrapMode::Repeat;
    WrapMode wrapV = WrapMode::Repeat;
    WrapMode wrapW = WrapMode::Repeat;
    float mipLodBias = 0.0f;
    float minLod = 0.0f;
    float maxLod = 1000.0f;
    float maxAnisotropy = 1.0f;
    bool compareMode = false;                  // 启用比较模式
    uint8_t compareFuncValue = 1;              // 比较函数值（对应CompareFunc::Less）
    float borderColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

/**
 * @brief 纹理描述符
 */
struct TextureDescriptor {
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;                        // 3D纹理深度或数组层数
    TextureType type = TextureType::Texture2D;
    PixelFormat format = PixelFormat::RGBA8;
    uint32_t mipLevels = 1;                    // Mipmap层数（0表示自动计算）
    uint32_t sampleCount = 1;                  // 多重采样数
    const void* data = nullptr;                // 初始像素数据
    SamplerDescriptor sampler;                 // 采样器配置
    bool generateMipmaps = false;              // 是否自动生成Mipmap
    const char* debugName = nullptr;
};

/**
 * @brief 纹理区域
 */
struct TextureRegion {
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t z = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;
    uint32_t mipLevel = 0;
    uint32_t arrayLayer = 0;
};

// =============================================================================
// 图像/平面数据格式（面向上层数据交互）
// =============================================================================

/**
 * @brief 通用图像格式（描述 CPU / GPU 之间传输的数据格式）
 */
enum class ImageFormat : uint8_t {
    YUV420P,
    NV12,
    NV21,
    RGBA8,
    BGRA8,
    RGB8,
    GRAY8,
    Unknown
};

/**
 * @brief 颜色空间
 */
enum class ColorSpace : uint8_t {
    BT709,
    BT601,
    BT2020,
    Unknown
};

/**
 * @brief 颜色范围
 */
enum class ColorRange : uint8_t {
    Video,   // 限幅（通常对应 [16,235]）
    Full,
    Unknown
};

/**
 * @brief 单个图像平面描述
 */
struct ImagePlaneDesc {
    const void* data = nullptr;
    uint32_t    stride = 0;  // 行字节数，0 表示紧密排列
};

/**
 * @brief 图像数据描述，用于 CPU 与 GPU 交互
 */
struct ImageDataDesc {
    uint32_t    width  = 0;
    uint32_t    height = 0;
    ImageFormat format = ImageFormat::Unknown;

    std::vector<ImagePlaneDesc> planes;

    ColorSpace  colorSpace = ColorSpace::BT709;
    ColorRange  range      = ColorRange::Video;
};

// =============================================================================
// 帧缓冲相关
// =============================================================================

/**
 * @brief 附件加载操作
 */
enum class LoadOp : uint8_t {
    Load,      // 保留内容
    Clear,     // 清除内容
    DontCare   // 不关心
};

/**
 * @brief 附件存储操作
 */
enum class StoreOp : uint8_t {
    Store,     // 存储内容
    DontCare   // 不关心
};

/**
 * @brief 颜色附件描述
 */
struct ColorAttachmentDescriptor {
    PixelFormat format = PixelFormat::RGBA8;
    LoadOp loadOp = LoadOp::Clear;
    StoreOp storeOp = StoreOp::Store;
    float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
};

/**
 * @brief 深度模板附件描述
 */
struct DepthStencilAttachmentDescriptor {
    PixelFormat format = PixelFormat::Depth24Stencil8;
    LoadOp depthLoadOp = LoadOp::Clear;
    StoreOp depthStoreOp = StoreOp::Store;
    LoadOp stencilLoadOp = LoadOp::DontCare;
    StoreOp stencilStoreOp = StoreOp::DontCare;
    float clearDepth = 1.0f;
    uint8_t clearStencil = 0;
};

/**
 * @brief 帧缓冲描述符
 */
struct FrameBufferDescriptor {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t samples = 1;
    std::vector<ColorAttachmentDescriptor> colorAttachments;
    DepthStencilAttachmentDescriptor depthStencilAttachment;
    bool hasDepthStencil = true;
    const char* debugName = nullptr;
};

// =============================================================================
// 管线状态相关
// =============================================================================

/**
 * @brief 图元类型
 */
enum class PrimitiveType : uint8_t {
    Points,
    Lines,
    LineStrip,
    Triangles,
    TriangleStrip,
    TriangleFan
};

/**
 * @brief 比较函数
 */
enum class CompareFunc : uint8_t {
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

/**
 * @brief 模板操作
 */
enum class StencilOp : uint8_t {
    Keep,
    Zero,
    Replace,
    Increment,
    IncrementWrap,
    Decrement,
    DecrementWrap,
    Invert
};

/**
 * @brief 混合因子
 */
enum class BlendFactor : uint8_t {
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    ConstantColor,
    OneMinusConstantColor,
    ConstantAlpha,
    OneMinusConstantAlpha,
    SrcAlphaSaturate
};

/**
 * @brief 混合操作
 */
enum class BlendOp : uint8_t {
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max
};

/**
 * @brief 剔除模式
 */
enum class CullMode : uint8_t {
    None,
    Front,
    Back,
    FrontAndBack
};

/**
 * @brief 正面朝向
 */
enum class FrontFace : uint8_t {
    CCW,  // 逆时针
    CW    // 顺时针
};

/**
 * @brief 多边形填充模式
 */
enum class PolygonMode : uint8_t {
    Fill,
    Line,
    Point
};

/**
 * @brief 填充模式（别名，与PolygonMode对应，用于渲染状态）
 */
enum class FillMode : uint8_t {
    Solid     = 0,  // 填充
    Wireframe = 1,  // 线框
    Point     = 2   // 点
};

/**
 * @brief 混合状态描述
 */
struct BlendStateDescriptor {
    bool enabled = false;
    BlendFactor srcColorFactor = BlendFactor::One;
    BlendFactor dstColorFactor = BlendFactor::Zero;
    BlendOp colorOp = BlendOp::Add;
    BlendFactor srcAlphaFactor = BlendFactor::One;
    BlendFactor dstAlphaFactor = BlendFactor::Zero;
    BlendOp alphaOp = BlendOp::Add;
    uint8_t colorWriteMask = 0x0F;  // RGBA
};

/**
 * @brief 模板面状态描述
 */
struct StencilFaceDescriptor {
    StencilOp failOp = StencilOp::Keep;
    StencilOp depthFailOp = StencilOp::Keep;
    StencilOp passOp = StencilOp::Keep;
    CompareFunc compareFunc = CompareFunc::Always;
};

/**
 * @brief 深度模板状态描述
 */
struct DepthStencilStateDescriptor {
    // 深度状态
    bool depthTestEnabled = true;
    bool depthWriteEnabled = true;
    CompareFunc depthCompareFunc = CompareFunc::Less;
    // 模板状态
    bool stencilEnabled = false;
    uint8_t stencilReadMask = 0xFF;
    uint8_t stencilWriteMask = 0xFF;
    uint8_t stencilRef = 0;
    StencilFaceDescriptor frontFace;
    StencilFaceDescriptor backFace;
};

/**
 * @brief 光栅化状态描述
 */
struct RasterizerStateDescriptor {
    CullMode cullMode = CullMode::Back;
    FrontFace frontFace = FrontFace::CCW;
    FillMode fillMode = FillMode::Solid;
    float depthBias = 0.0f;
    float depthBiasSlopeFactor = 0.0f;
    float depthBiasClamp = 0.0f;
    bool depthBiasEnabled = false;
    bool depthClipEnabled = true;
    bool scissorEnabled = false;
    bool multisampleEnabled = false;
    bool antialiasedLineEnabled = false;
    float lineWidth = 1.0f;
};

/**
 * @brief 管线状态描述符
 */
struct PipelineStateDescriptor {
    LRShader* vertexShader = nullptr;
    LRShader* fragmentShader = nullptr;
    LRShader* geometryShader = nullptr;
    VertexLayoutDescriptor vertexLayout;
    BlendStateDescriptor blendState;
    DepthStencilStateDescriptor depthStencilState;
    RasterizerStateDescriptor rasterizerState;
    PrimitiveType primitiveType = PrimitiveType::Triangles;
    uint32_t sampleCount = 1;
    const char* debugName = nullptr;
};

// =============================================================================
// 渲染通道相关
// =============================================================================

/**
 * @brief 视口
 */
struct Viewport {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float minDepth = 0.0f;
    float maxDepth = 1.0f;
};

/**
 * @brief 裁剪矩形
 */
struct ScissorRect {
    int32_t x = 0;
    int32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

/**
 * @brief 清除标志
 */
enum ClearFlags : uint8_t {
    ClearNone    = 0,
    ClearColor   = LR_BIT(0),
    ClearDepth   = LR_BIT(1),
    ClearStencil = LR_BIT(2),
    ClearAll     = ClearColor | ClearDepth | ClearStencil
};

// 清除标志别名（下划线格式）
constexpr uint8_t ClearFlag_Color   = ClearColor;
constexpr uint8_t ClearFlag_Depth   = ClearDepth;
constexpr uint8_t ClearFlag_Stencil = ClearStencil;
constexpr uint8_t ClearFlag_All     = ClearAll;

/**
 * @brief 颜色写入掩码
 */
enum ColorMask : uint8_t {
    ColorMask_None = 0,
    ColorMask_R    = LR_BIT(0),
    ColorMask_G    = LR_BIT(1),
    ColorMask_B    = LR_BIT(2),
    ColorMask_A    = LR_BIT(3),
    ColorMask_All  = ColorMask_R | ColorMask_G | ColorMask_B | ColorMask_A
};

// =============================================================================
// 同步相关
// =============================================================================

/**
 * @brief 栅栏状态
 */
enum class FenceStatus : uint8_t {
    Unsignaled,  // 未触发
    Signaled,    // 已触发
    Error        // 错误
};

// =============================================================================
// 渲染上下文相关
// =============================================================================

/**
 * @brief 渲染上下文描述符
 */
struct RenderContextDescriptor {
    Backend backend = Backend::OpenGL;
    void* windowHandle = nullptr;              // 平台窗口句柄
    uint32_t width = 800;
    uint32_t height = 600;
    bool vsync = true;
    bool debug = false;                        // 启用调试层
    uint32_t sampleCount = 1;
    const char* applicationName = "LREngine";
};

// =============================================================================
// 资源句柄
// =============================================================================

/**
 * @brief 通用资源句柄
 */
union ResourceHandle {
    void* ptr;
    uint32_t glHandle;       // OpenGL句柄
    uint64_t uint64Handle;
    
    ResourceHandle() : uint64Handle(0) {}
    explicit ResourceHandle(void* p) : ptr(p) {}
    explicit ResourceHandle(uint32_t handle) : glHandle(handle) {}
    explicit ResourceHandle(uint64_t handle) : uint64Handle(handle) {}
    
    bool IsValid() const { return uint64Handle != 0; }
};

// =============================================================================
// 工具函数
// =============================================================================

/**
 * @brief 获取顶点格式的字节大小
 */
inline uint32_t GetVertexFormatSize(VertexFormat format) {
    switch (format) {
        case VertexFormat::Float:     return 4;
        case VertexFormat::Float2:    return 8;
        case VertexFormat::Float3:    return 12;
        case VertexFormat::Float4:    return 16;
        case VertexFormat::Int:       return 4;
        case VertexFormat::Int2:      return 8;
        case VertexFormat::Int3:      return 12;
        case VertexFormat::Int4:      return 16;
        case VertexFormat::UInt:      return 4;
        case VertexFormat::UInt2:     return 8;
        case VertexFormat::UInt3:     return 12;
        case VertexFormat::UInt4:     return 16;
        case VertexFormat::Short2:    return 4;
        case VertexFormat::Short4:    return 8;
        case VertexFormat::UShort2:   return 4;
        case VertexFormat::UShort4:   return 8;
        case VertexFormat::Byte4:     return 4;
        case VertexFormat::UByte4:    return 4;
        case VertexFormat::Byte4Norm: return 4;
        case VertexFormat::UByte4Norm:return 4;
        default: return 0;
    }
}

/**
 * @brief 获取像素格式的字节大小
 */
inline uint32_t GetPixelFormatSize(PixelFormat format) {
    switch (format) {
        case PixelFormat::R8:           return 1;
        case PixelFormat::RG8:          return 2;
        case PixelFormat::RGB8:         return 3;
        case PixelFormat::RGBA8:        return 4;
        case PixelFormat::R16F:         return 2;
        case PixelFormat::RG16F:        return 4;
        case PixelFormat::RGB16F:       return 6;
        case PixelFormat::RGBA16F:      return 8;
        case PixelFormat::R32F:         return 4;
        case PixelFormat::RG32F:        return 8;
        case PixelFormat::RGB32F:       return 12;
        case PixelFormat::RGBA32F:      return 16;
        case PixelFormat::RGB10A2:      return 4;
        case PixelFormat::Depth16:      return 2;
        case PixelFormat::Depth24:      return 3;
        case PixelFormat::Depth32F:     return 4;
        case PixelFormat::Depth24Stencil8: return 4;
        case PixelFormat::Depth32FStencil8: return 5;
        case PixelFormat::Stencil8:     return 1;
        default: return 0;  // 压缩格式返回0
    }
}

/**
 * @brief 检查像素格式是否为深度格式
 */
inline bool IsDepthFormat(PixelFormat format) {
    switch (format) {
        case PixelFormat::Depth16:
        case PixelFormat::Depth24:
        case PixelFormat::Depth32F:
        case PixelFormat::Depth24Stencil8:
        case PixelFormat::Depth32FStencil8:
            return true;
        default:
            return false;
    }
}

/**
 * @brief 检查像素格式是否包含模板
 */
inline bool HasStencil(PixelFormat format) {
    switch (format) {
        case PixelFormat::Depth24Stencil8:
        case PixelFormat::Depth32FStencil8:
        case PixelFormat::Stencil8:
            return true;
        default:
            return false;
    }
}

} // namespace render
} // namespace lrengine
