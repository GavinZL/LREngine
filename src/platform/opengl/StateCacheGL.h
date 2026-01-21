/**
 * @file StateCacheGL.h
 * @brief OpenGL状态缓存
 * 
 * 通过缓存OpenGL状态来减少冗余的状态切换调用，提升渲染性能
 */

#pragma once

#include "TypeConverterGL.h"
#include <array>

#ifdef LRENGINE_ENABLE_OPENGL

namespace lrengine {
namespace render {
namespace gl {

/**
 * @brief OpenGL状态缓存
 * 
 * 缓存当前OpenGL上下文的状态，避免不必要的状态切换
 */
class StateCacheGL {
public:
    static constexpr uint32_t MAX_TEXTURE_UNITS   = 16;
    static constexpr uint32_t MAX_VERTEX_ATTRIBS  = 16;
    static constexpr uint32_t MAX_UNIFORM_BUFFERS = 16;

    StateCacheGL();
    ~StateCacheGL() = default;

    // 重置所有缓存状态
    void Reset();

    // 使缓存失效（通常在上下文切换后调用）
    void Invalidate();

    // 程序
    void UseProgram(GLuint program);
    GLuint GetCurrentProgram() const { return m_currentProgram; }

    // 顶点数组对象
    void BindVAO(GLuint vao);
    GLuint GetCurrentVAO() const { return m_currentVAO; }

    // 缓冲区
    void BindBuffer(GLenum target, GLuint buffer);
    void BindBufferBase(GLenum target, GLuint index, GLuint buffer);

    // 纹理
    void SetActiveTexture(uint32_t unit);
    void BindTexture(GLenum target, GLuint texture);
    void BindTextureUnit(uint32_t unit, GLenum target, GLuint texture);

    // 帧缓冲
    void BindFramebuffer(GLenum target, GLuint framebuffer);
    GLuint GetCurrentFramebuffer() const { return m_currentFramebuffer; }

    // 视口和裁剪
    void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height);
    void SetScissor(int32_t x, int32_t y, int32_t width, int32_t height);

    // 深度状态
    void SetDepthTest(bool enabled);
    void SetDepthWrite(bool enabled);
    void SetDepthFunc(GLenum func);

    // 模板状态
    void SetStencilTest(bool enabled);

    // 混合状态
    void SetBlend(bool enabled);
    void SetBlendFunc(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
    void SetBlendEquation(GLenum modeRGB, GLenum modeAlpha);
    void SetColorMask(bool r, bool g, bool b, bool a);

    // 面剔除
    void SetCullFace(bool enabled);
    void SetCullMode(GLenum mode);
    void SetFrontFace(GLenum mode);

    // 多边形模式
    void SetPolygonMode(GLenum mode);

    // 裁剪测试
    void SetScissorTest(bool enabled);

    // 多采样
    void SetMultisample(bool enabled);

    // 线宽
    void SetLineWidth(float width);

    // 清除颜色
    void SetClearColor(float r, float g, float b, float a);
    void SetClearDepth(float depth);
    void SetClearStencil(int32_t stencil);

private:
    // 程序状态
    GLuint m_currentProgram;

    // VAO状态
    GLuint m_currentVAO;

    // 缓冲区状态
    GLuint m_arrayBuffer;
    GLuint m_elementArrayBuffer;
    GLuint m_uniformBuffer;
    std::array<GLuint, MAX_UNIFORM_BUFFERS> m_uniformBufferBindings;

    // 纹理状态
    uint32_t m_activeTextureUnit;
    std::array<GLuint, MAX_TEXTURE_UNITS> m_boundTextures2D;
    std::array<GLuint, MAX_TEXTURE_UNITS> m_boundTexturesCube;

    // 帧缓冲状态
    GLuint m_currentFramebuffer;
    GLuint m_currentReadFramebuffer;
    GLuint m_currentDrawFramebuffer;

    // 视口状态
    struct {
        int32_t x, y, width, height;
    } m_viewport;

    // 裁剪状态
    struct {
        int32_t x, y, width, height;
    } m_scissor;

    // 深度状态
    bool m_depthTestEnabled;
    bool m_depthWriteEnabled;
    GLenum m_depthFunc;

    // 模板状态
    bool m_stencilTestEnabled;

    // 混合状态
    bool m_blendEnabled;
    GLenum m_blendSrcRGB, m_blendDstRGB;
    GLenum m_blendSrcAlpha, m_blendDstAlpha;
    GLenum m_blendModeRGB, m_blendModeAlpha;
    bool m_colorMaskR, m_colorMaskG, m_colorMaskB, m_colorMaskA;

    // 面剔除状态
    bool m_cullFaceEnabled;
    GLenum m_cullMode;
    GLenum m_frontFace;

    // 多边形模式
    GLenum m_polygonMode;

    // 其他状态
    bool m_scissorTestEnabled;
    bool m_multisampleEnabled;
    float m_lineWidth;

    // 清除值
    float m_clearColor[4];
    float m_clearDepth;
    int32_t m_clearStencil;

    // 有效性标志
    bool m_valid;
};

} // namespace gl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGL
