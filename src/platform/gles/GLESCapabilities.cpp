/**
 * @file GLESCapabilities.cpp
 * @brief OpenGL ES能力检测实现
 */

#include "GLESCapabilities.h"
#include "lrengine/utils/LRLog.h"
#include <cstring>

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

void GLESCapabilities::Initialize() {
    // 获取基本信息
    const char* vendorStr = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const char* rendererStr = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const char* versionStr = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const char* glslVersionStr = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    
    if (vendorStr) vendor = vendorStr;
    if (rendererStr) renderer = rendererStr;
    if (versionStr) versionString = versionStr;
    
    // 获取版本号
    glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
    
    // 解析GLSL版本
    if (glslVersionStr) {
        // GLSL ES版本字符串格式: "OpenGL ES GLSL ES 3.00"
        const char* version = strstr(glslVersionStr, "ES ");
        if (version) {
            version += 3; // 跳过 "ES "
            int major = 0, minor = 0;
            if (sscanf(version, "%d.%d", &major, &minor) == 2) {
                glslVersion = major * 100 + minor;
            }
        }
    }
    
    // 获取扩展字符串
    const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (extensions) {
        m_extensionsString = extensions;
    } else {
        // ES 3.0+ 可能需要使用indexed查询
        GLint numExtensions = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
        for (GLint i = 0; i < numExtensions; ++i) {
            const char* ext = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
            if (ext) {
                m_extensionsString += ext;
                m_extensionsString += " ";
            }
        }
    }
    
    // 检测各种能力
    DetectVersionFeatures();
    DetectExtensions();
    QueryLimits();
    DetectShaderPrecision();
    
    // 输出检测结果
    LogCapabilities();
}

bool GLESCapabilities::HasExtension(const char* extensionName) const {
    if (!extensionName || m_extensionsString.empty()) {
        return false;
    }
    
    // 确保完整匹配扩展名（避免GL_EXT_test匹配GL_EXT_test_extended）
    std::string searchStr = std::string(" ") + extensionName + " ";
    std::string paddedExtensions = " " + m_extensionsString + " ";
    
    return paddedExtensions.find(searchStr) != std::string::npos;
}

void GLESCapabilities::DetectVersionFeatures() {
    // ES 3.0 核心功能（默认已支持）
    hasMapBufferRange = true;
    hasTextureStorage = true;
    hasVertexArrayObject = true;
    hasUniformBufferObject = true;
    hasInstancingBase = true;
    hasSamplerObjects = true;
    hasETCCompression = true;
    hasTextureFloat = true;
    hasTextureHalfFloat = true;
    hasTextureHalfFloatLinear = true;
    hasDepthTexture = true;
    hasRGB8RGBA8 = true;
    
    // ES 3.1+ 功能
    if (majorVersion > 3 || (majorVersion == 3 && minorVersion >= 1)) {
        hasComputeShader = true;
        hasSSBO = true;
        hasIndirectDraw = true;
        hasMultisampleTexture = true;
        hasProgramPipeline = true;
    }
    
    // ES 3.2 功能
    if (majorVersion > 3 || (majorVersion == 3 && minorVersion >= 2)) {
        hasGeometryShader = true;
        hasTessellationShader = true;
        hasDebugOutput = true;
    }
}

void GLESCapabilities::DetectExtensions() {
    // 纹理压缩
    hasASTCCompression = HasExtension("GL_KHR_texture_compression_astc_ldr") ||
                         HasExtension("GL_KHR_texture_compression_astc_hdr");
    hasS3TCCompression = HasExtension("GL_EXT_texture_compression_s3tc") ||
                         HasExtension("GL_EXT_texture_compression_dxt1");
    hasPVRTCCompression = HasExtension("GL_IMG_texture_compression_pvrtc");
    
    // 纹理采样
    hasAnisotropicFiltering = HasExtension("GL_EXT_texture_filter_anisotropic");
    hasTextureBorderClamp = HasExtension("GL_EXT_texture_border_clamp") ||
                            HasExtension("GL_OES_texture_border_clamp");
    hasTextureFloatLinear = HasExtension("GL_OES_texture_float_linear");
    
    // 渲染
    hasColorBufferFloat = HasExtension("GL_EXT_color_buffer_float") ||
                          HasExtension("GL_EXT_color_buffer_half_float");
    hasDiscardFramebuffer = HasExtension("GL_EXT_discard_framebuffer");
    hasDrawBuffersIndexed = HasExtension("GL_EXT_draw_buffers_indexed");
    hasBlendFuncExtended = HasExtension("GL_EXT_blend_func_extended");
    
    // 绘制
    hasMultiDrawIndirect = HasExtension("GL_EXT_multi_draw_indirect");
    
    // 其他
    hasClipControl = HasExtension("GL_EXT_clip_control");
    
    // 调试（可能通过扩展获得）
    if (!hasDebugOutput) {
        hasDebugOutput = HasExtension("GL_KHR_debug");
    }
}

void GLESCapabilities::QueryLimits() {
    // 纹理限制
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &maxCubeMapTextureSize);
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max3DTextureSize);
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxArrayTextureLayers);
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombinedTextureUnits);
    
    // 顶点限制
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &maxVertexTextureImageUnits);
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &maxVertexUniformVectors);
    maxVertexUniformComponents = maxVertexUniformVectors * 4;
    
    // 片段限制
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &maxFragmentUniformVectors);
    maxFragmentUniformComponents = maxFragmentUniformVectors * 4;
    
    // Uniform缓冲限制
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxUniformBufferBindings);
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockSize);
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &maxVertexUniformBlocks);
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &maxFragmentUniformBlocks);
    
    // 帧缓冲限制
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxRenderbufferSize);
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, maxViewportDims);
    
    // 各向异性过滤
    if (hasAnisotropicFiltering) {
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
    }
}

void GLESCapabilities::DetectShaderPrecision() {
    // 检查片段着色器的浮点精度支持
    GLint range[2], precision;
    
    // 检查highp float
    glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_HIGH_FLOAT, range, &precision);
    hasHighPrecisionFloat = (precision > 0);
    
    // 检查mediump float
    glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_MEDIUM_FLOAT, range, &precision);
    hasMediumPrecisionFloat = (precision > 0);
}

void GLESCapabilities::LogCapabilities() const {
    LR_LOG_INFO("======================================");
    LR_LOG_INFO("OpenGL ES Capabilities Report");
    LR_LOG_INFO("======================================");
    
    // 版本信息
    LR_LOG_INFO_F("Vendor: %s", vendor.c_str());
    LR_LOG_INFO_F("Renderer: %s", renderer.c_str());
    LR_LOG_INFO_F("Version: %s", versionString.c_str());
    LR_LOG_INFO_F("GL ES Version: %d.%d", majorVersion, minorVersion);
    LR_LOG_INFO_F("GLSL ES Version: %d", glslVersion);
    
    // 关键功能
    LR_LOG_INFO("--- Key Features ---");
    LR_LOG_INFO_F("Compute Shader: %s", hasComputeShader ? "Yes" : "No");
    LR_LOG_INFO_F("SSBO: %s", hasSSBO ? "Yes" : "No");
    LR_LOG_INFO_F("Geometry Shader: %s", hasGeometryShader ? "Yes" : "No");
    LR_LOG_INFO_F("Tessellation: %s", hasTessellationShader ? "Yes" : "No");
    
    // 纹理
    LR_LOG_INFO("--- Texture ---");
    LR_LOG_INFO_F("Max Texture Size: %d", maxTextureSize);
    LR_LOG_INFO_F("ASTC Compression: %s", hasASTCCompression ? "Yes" : "No");
    LR_LOG_INFO_F("S3TC Compression: %s", hasS3TCCompression ? "Yes" : "No");
    LR_LOG_INFO_F("Anisotropic Filtering: %s (max: %.1f)", 
                  hasAnisotropicFiltering ? "Yes" : "No", maxAnisotropy);
    LR_LOG_INFO_F("Border Clamp: %s", hasTextureBorderClamp ? "Yes" : "No");
    LR_LOG_INFO_F("Float Texture Linear: %s", hasTextureFloatLinear ? "Yes" : "No");
    LR_LOG_INFO_F("Color Buffer Float: %s", hasColorBufferFloat ? "Yes" : "No");
    
    // 精度
    LR_LOG_INFO("--- Shader Precision ---");
    LR_LOG_INFO_F("Fragment highp float: %s", hasHighPrecisionFloat ? "Yes" : "No");
    LR_LOG_INFO_F("Fragment mediump float: %s", hasMediumPrecisionFloat ? "Yes" : "No");
    
    // 限制
    LR_LOG_INFO("--- Limits ---");
    LR_LOG_INFO_F("Max Vertex Attribs: %d", maxVertexAttribs);
    LR_LOG_INFO_F("Max Texture Units: %d", maxTextureUnits);
    LR_LOG_INFO_F("Max Uniform Blocks: VS=%d, FS=%d", maxVertexUniformBlocks, maxFragmentUniformBlocks);
    LR_LOG_INFO_F("Max Color Attachments: %d", maxColorAttachments);
    LR_LOG_INFO_F("Max Samples: %d", maxSamples);
    
    LR_LOG_INFO("======================================");
}

} // namespace gles
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGLES
