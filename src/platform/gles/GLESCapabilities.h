/**
 * @file GLESCapabilities.h
 * @brief OpenGL ES能力检测与扩展查询
 */

#pragma once

#include "TypeConverterGLES.h"
#include <string>

#ifdef LRENGINE_ENABLE_OPENGLES

namespace lrengine {
namespace render {
namespace gles {

/**
 * @brief OpenGL ES能力检测类
 * 
 * 用于检测设备支持的OpenGL ES版本、扩展和硬件限制
 */
class GLESCapabilities {
public:
    GLESCapabilities()  = default;
    ~GLESCapabilities() = default;

    /**
     * @brief 初始化能力检测
     * @note 必须在OpenGL ES上下文创建后调用
     */
    void Initialize();

    /**
     * @brief 检查是否支持指定扩展
     * @param extensionName 扩展名称
     * @return 支持返回true
     */
    bool HasExtension(const char* extensionName) const;

    /**
     * @brief 打印所有能力信息到日志
     */
    void LogCapabilities() const;

public:
    // =========================================================================
    // 版本信息
    // =========================================================================
    int majorVersion = 3;      // 主版本号
    int minorVersion = 0;      // 次版本号
    int glslVersion  = 300;    // GLSL ES版本 (300 = 3.00)
    std::string vendor;        // 厂商
    std::string renderer;      // 渲染器名称
    std::string versionString; // 完整版本字符串

    // =========================================================================
    // 扩展支持
    // =========================================================================

    // 核心功能 (ES 3.0)
    bool hasMapBufferRange      = true; // glMapBufferRange (ES 3.0原生)
    bool hasTextureStorage      = true; // glTexStorage2D (ES 3.0原生)
    bool hasVertexArrayObject   = true; // VAO (ES 3.0原生)
    bool hasUniformBufferObject = true; // UBO (ES 3.0原生)
    bool hasInstancingBase      = true; // 基础实例化 (ES 3.0原生)
    bool hasSamplerObjects      = true; // 采样器对象 (ES 3.0原生)

    // ES 3.1+ 功能
    bool hasComputeShader      = false; // 计算着色器 (ES 3.1+)
    bool hasSSBO               = false; // Shader Storage Buffer (ES 3.1+)
    bool hasIndirectDraw       = false; // 间接绘制 (ES 3.1+)
    bool hasMultisampleTexture = false; // 多采样纹理 (ES 3.1+)
    bool hasProgramPipeline    = false; // 分离着色器程序 (ES 3.1+)

    // ES 3.2 功能
    bool hasGeometryShader     = false; // 几何着色器 (ES 3.2)
    bool hasTessellationShader = false; // 曲面细分着色器 (ES 3.2)
    bool hasDebugOutput        = false; // 调试输出 (ES 3.2 或扩展)

    // 纹理相关扩展
    bool hasETCCompression       = true;   // ETC2压缩 (ES 3.0原生)
    bool hasASTCCompression      = false;  // ASTC压缩 (GL_KHR_texture_compression_astc_ldr)
    bool hasS3TCCompression      = false;  // S3TC/DXT压缩 (GL_EXT_texture_compression_s3tc)
    bool hasPVRTCCompression     = false;  // PVRTC压缩 (GL_IMG_texture_compression_pvrtc)
    bool hasAnisotropicFiltering = false;  // 各向异性过滤 (GL_EXT_texture_filter_anisotropic)
    bool hasTextureBorderClamp   = false;  // 边界钳制 (GL_EXT_texture_border_clamp)
    bool hasTextureFloat         = true;   // 浮点纹理 (ES 3.0原生)
    bool hasTextureFloatLinear   = false;  // 浮点纹理线性过滤 (GL_OES_texture_float_linear)
    bool hasTextureHalfFloat     = true;   // 半精度浮点纹理 (ES 3.0原生)
    bool hasTextureHalfFloatLinear = true; // 半精度浮点纹理线性过滤 (ES 3.0原生)
    bool hasDepthTexture           = true; // 深度纹理 (ES 3.0原生)
    bool hasColorBufferFloat       = false; // 浮点颜色缓冲渲染 (GL_EXT_color_buffer_float)
    bool hasRGB8RGBA8              = true;  // RGB8/RGBA8内部格式 (ES 3.0原生)

    // 其他扩展
    bool hasDiscardFramebuffer = false; // 丢弃帧缓冲 (GL_EXT_discard_framebuffer)
    bool hasDrawBuffersIndexed = false; // MRT独立混合 (GL_EXT_draw_buffers_indexed)
    bool hasBlendFuncExtended  = false; // 扩展混合函数 (GL_EXT_blend_func_extended)
    bool hasMultiDrawIndirect  = false; // 多重间接绘制 (GL_EXT_multi_draw_indirect)
    bool hasClipControl        = false; // 裁剪控制 (GL_EXT_clip_control)

    // =========================================================================
    // 硬件限制
    // =========================================================================

    // 纹理
    int maxTextureSize          = 2048;
    int maxCubeMapTextureSize   = 2048;
    int max3DTextureSize        = 256;
    int maxArrayTextureLayers   = 256;
    int maxTextureUnits         = 16; // 片段着色器纹理单元
    int maxCombinedTextureUnits = 32; // 总纹理单元数
    int maxTextureImageUnits    = 16;

    // 顶点
    int maxVertexAttribs           = 16;
    int maxVertexTextureImageUnits = 16;
    int maxVertexUniformVectors    = 256;
    int maxVertexUniformComponents = 1024;

    // 片段
    int maxFragmentUniformVectors    = 224;
    int maxFragmentUniformComponents = 896;

    // Uniform缓冲
    int maxUniformBufferBindings = 24;
    int maxUniformBlockSize      = 16384;
    int maxVertexUniformBlocks   = 12;
    int maxFragmentUniformBlocks = 12;

    // 帧缓冲
    int maxColorAttachments = 4;
    int maxDrawBuffers      = 4;
    int maxRenderbufferSize = 2048;
    int maxSamples          = 4;
    int maxViewportDims[2]  = {2048, 2048};

    // 各向异性
    float maxAnisotropy = 1.0f;

    // 精度
    bool hasHighPrecisionFloat   = true; // 片段着色器是否支持highp float
    bool hasMediumPrecisionFloat = true; // 片段着色器是否支持mediump float

private:
    std::string m_extensionsString; // 缓存的扩展字符串

    /**
     * @brief 检测ES版本相关功能
     */
    void DetectVersionFeatures();

    /**
     * @brief 检测扩展
     */
    void DetectExtensions();

    /**
     * @brief 查询硬件限制
     */
    void QueryLimits();

    /**
     * @brief 检测着色器精度
     */
    void DetectShaderPrecision();
};

} // namespace gles
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGLES
