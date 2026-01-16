/**
 * @file StateCacheGL.cpp
 * @brief OpenGL状态缓存实现
 */

#include "StateCacheGL.h"

#ifdef LRENGINE_ENABLE_OPENGL

namespace lrengine {
namespace render {
namespace gl {

StateCacheGL::StateCacheGL()
{
    Reset();
}

void StateCacheGL::Reset()
{
    m_currentProgram = 0;
    m_currentVAO = 0;
    m_arrayBuffer = 0;
    m_elementArrayBuffer = 0;
    m_uniformBuffer = 0;
    m_uniformBufferBindings.fill(0);
    
    m_activeTextureUnit = 0;
    m_boundTextures2D.fill(0);
    m_boundTexturesCube.fill(0);

    m_currentFramebuffer = 0;
    m_currentReadFramebuffer = 0;
    m_currentDrawFramebuffer = 0;

    m_viewport = {0, 0, 0, 0};
    m_scissor = {0, 0, 0, 0};

    m_depthTestEnabled = false;
    m_depthWriteEnabled = true;
    m_depthFunc = GL_LESS;

    m_stencilTestEnabled = false;

    m_blendEnabled = false;
    m_blendSrcRGB = GL_ONE;
    m_blendDstRGB = GL_ZERO;
    m_blendSrcAlpha = GL_ONE;
    m_blendDstAlpha = GL_ZERO;
    m_blendModeRGB = GL_FUNC_ADD;
    m_blendModeAlpha = GL_FUNC_ADD;
    m_colorMaskR = m_colorMaskG = m_colorMaskB = m_colorMaskA = true;

    m_cullFaceEnabled = false;
    m_cullMode = GL_BACK;
    m_frontFace = GL_CCW;

    m_polygonMode = GL_FILL;
    m_scissorTestEnabled = false;
    m_multisampleEnabled = true;
    m_lineWidth = 1.0f;

    m_clearColor[0] = m_clearColor[1] = m_clearColor[2] = 0.0f;
    m_clearColor[3] = 1.0f;
    m_clearDepth = 1.0f;
    m_clearStencil = 0;

    m_valid = true;
}

void StateCacheGL::Invalidate()
{
    m_valid = false;
    Reset();
}

void StateCacheGL::UseProgram(GLuint program)
{
    if (!m_valid || m_currentProgram != program) {
        glUseProgram(program);
        m_currentProgram = program;
    }
}

void StateCacheGL::BindVAO(GLuint vao)
{
    if (!m_valid || m_currentVAO != vao) {
        glBindVertexArray(vao);
        m_currentVAO = vao;
    }
}

void StateCacheGL::BindBuffer(GLenum target, GLuint buffer)
{
    GLuint* cached = nullptr;
    
    switch (target) {
        case GL_ARRAY_BUFFER:
            cached = &m_arrayBuffer;
            break;
        case GL_ELEMENT_ARRAY_BUFFER:
            cached = &m_elementArrayBuffer;
            break;
        case GL_UNIFORM_BUFFER:
            cached = &m_uniformBuffer;
            break;
        default:
            glBindBuffer(target, buffer);
            return;
    }

    if (!m_valid || *cached != buffer) {
        glBindBuffer(target, buffer);
        *cached = buffer;
    }
}

void StateCacheGL::BindBufferBase(GLenum target, GLuint index, GLuint buffer)
{
    if (target == GL_UNIFORM_BUFFER && index < MAX_UNIFORM_BUFFERS) {
        if (!m_valid || m_uniformBufferBindings[index] != buffer) {
            glBindBufferBase(target, index, buffer);
            m_uniformBufferBindings[index] = buffer;
        }
    } else {
        glBindBufferBase(target, index, buffer);
    }
}

void StateCacheGL::SetActiveTexture(uint32_t unit)
{
    if (!m_valid || m_activeTextureUnit != unit) {
        glActiveTexture(GL_TEXTURE0 + unit);
        m_activeTextureUnit = unit;
    }
}

void StateCacheGL::BindTexture(GLenum target, GLuint texture)
{
    uint32_t unit = m_activeTextureUnit;
    
    if (unit < MAX_TEXTURE_UNITS) {
        GLuint* cached = nullptr;
        
        switch (target) {
            case GL_TEXTURE_2D:
                cached = &m_boundTextures2D[unit];
                break;
            case GL_TEXTURE_CUBE_MAP:
                cached = &m_boundTexturesCube[unit];
                break;
            default:
                glBindTexture(target, texture);
                return;
        }

        if (!m_valid || *cached != texture) {
            glBindTexture(target, texture);
            *cached = texture;
        }
    } else {
        glBindTexture(target, texture);
    }
}

void StateCacheGL::BindTextureUnit(uint32_t unit, GLenum target, GLuint texture)
{
    SetActiveTexture(unit);
    BindTexture(target, texture);
}

void StateCacheGL::BindFramebuffer(GLenum target, GLuint framebuffer)
{
    GLuint* cached = nullptr;
    
    switch (target) {
        case GL_FRAMEBUFFER:
            if (!m_valid || m_currentFramebuffer != framebuffer) {
                glBindFramebuffer(target, framebuffer);
                m_currentFramebuffer = framebuffer;
                m_currentReadFramebuffer = framebuffer;
                m_currentDrawFramebuffer = framebuffer;
            }
            return;
        case GL_READ_FRAMEBUFFER:
            cached = &m_currentReadFramebuffer;
            break;
        case GL_DRAW_FRAMEBUFFER:
            cached = &m_currentDrawFramebuffer;
            break;
        default:
            glBindFramebuffer(target, framebuffer);
            return;
    }

    if (!m_valid || *cached != framebuffer) {
        glBindFramebuffer(target, framebuffer);
        *cached = framebuffer;
    }
}

void StateCacheGL::SetViewport(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (!m_valid || m_viewport.x != x || m_viewport.y != y ||
        m_viewport.width != width || m_viewport.height != height) {
        glViewport(x, y, width, height);
        m_viewport = {x, y, width, height};
    }
}

void StateCacheGL::SetScissor(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (!m_valid || m_scissor.x != x || m_scissor.y != y ||
        m_scissor.width != width || m_scissor.height != height) {
        glScissor(x, y, width, height);
        m_scissor = {x, y, width, height};
    }
}

void StateCacheGL::SetDepthTest(bool enabled)
{
    if (!m_valid || m_depthTestEnabled != enabled) {
        if (enabled) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
        m_depthTestEnabled = enabled;
    }
}

void StateCacheGL::SetDepthWrite(bool enabled)
{
    if (!m_valid || m_depthWriteEnabled != enabled) {
        glDepthMask(enabled ? GL_TRUE : GL_FALSE);
        m_depthWriteEnabled = enabled;
    }
}

void StateCacheGL::SetDepthFunc(GLenum func)
{
    if (!m_valid || m_depthFunc != func) {
        glDepthFunc(func);
        m_depthFunc = func;
    }
}

void StateCacheGL::SetStencilTest(bool enabled)
{
    if (!m_valid || m_stencilTestEnabled != enabled) {
        if (enabled) {
            glEnable(GL_STENCIL_TEST);
        } else {
            glDisable(GL_STENCIL_TEST);
        }
        m_stencilTestEnabled = enabled;
    }
}

void StateCacheGL::SetBlend(bool enabled)
{
    if (!m_valid || m_blendEnabled != enabled) {
        if (enabled) {
            glEnable(GL_BLEND);
        } else {
            glDisable(GL_BLEND);
        }
        m_blendEnabled = enabled;
    }
}

void StateCacheGL::SetBlendFunc(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
    if (!m_valid || m_blendSrcRGB != srcRGB || m_blendDstRGB != dstRGB ||
        m_blendSrcAlpha != srcAlpha || m_blendDstAlpha != dstAlpha) {
        glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
        m_blendSrcRGB = srcRGB;
        m_blendDstRGB = dstRGB;
        m_blendSrcAlpha = srcAlpha;
        m_blendDstAlpha = dstAlpha;
    }
}

void StateCacheGL::SetBlendEquation(GLenum modeRGB, GLenum modeAlpha)
{
    if (!m_valid || m_blendModeRGB != modeRGB || m_blendModeAlpha != modeAlpha) {
        glBlendEquationSeparate(modeRGB, modeAlpha);
        m_blendModeRGB = modeRGB;
        m_blendModeAlpha = modeAlpha;
    }
}

void StateCacheGL::SetColorMask(bool r, bool g, bool b, bool a)
{
    if (!m_valid || m_colorMaskR != r || m_colorMaskG != g ||
        m_colorMaskB != b || m_colorMaskA != a) {
        glColorMask(r ? GL_TRUE : GL_FALSE, g ? GL_TRUE : GL_FALSE,
                   b ? GL_TRUE : GL_FALSE, a ? GL_TRUE : GL_FALSE);
        m_colorMaskR = r;
        m_colorMaskG = g;
        m_colorMaskB = b;
        m_colorMaskA = a;
    }
}

void StateCacheGL::SetCullFace(bool enabled)
{
    if (!m_valid || m_cullFaceEnabled != enabled) {
        if (enabled) {
            glEnable(GL_CULL_FACE);
        } else {
            glDisable(GL_CULL_FACE);
        }
        m_cullFaceEnabled = enabled;
    }
}

void StateCacheGL::SetCullMode(GLenum mode)
{
    if (!m_valid || m_cullMode != mode) {
        glCullFace(mode);
        m_cullMode = mode;
    }
}

void StateCacheGL::SetFrontFace(GLenum mode)
{
    if (!m_valid || m_frontFace != mode) {
        glFrontFace(mode);
        m_frontFace = mode;
    }
}

void StateCacheGL::SetPolygonMode(GLenum mode)
{
    if (!m_valid || m_polygonMode != mode) {
        glPolygonMode(GL_FRONT_AND_BACK, mode);
        m_polygonMode = mode;
    }
}

void StateCacheGL::SetScissorTest(bool enabled)
{
    if (!m_valid || m_scissorTestEnabled != enabled) {
        if (enabled) {
            glEnable(GL_SCISSOR_TEST);
        } else {
            glDisable(GL_SCISSOR_TEST);
        }
        m_scissorTestEnabled = enabled;
    }
}

void StateCacheGL::SetMultisample(bool enabled)
{
    if (!m_valid || m_multisampleEnabled != enabled) {
        if (enabled) {
            glEnable(GL_MULTISAMPLE);
        } else {
            glDisable(GL_MULTISAMPLE);
        }
        m_multisampleEnabled = enabled;
    }
}

void StateCacheGL::SetLineWidth(float width)
{
    if (!m_valid || m_lineWidth != width) {
        glLineWidth(width);
        m_lineWidth = width;
    }
}

void StateCacheGL::SetClearColor(float r, float g, float b, float a)
{
    if (!m_valid || m_clearColor[0] != r || m_clearColor[1] != g ||
        m_clearColor[2] != b || m_clearColor[3] != a) {
        glClearColor(r, g, b, a);
        m_clearColor[0] = r;
        m_clearColor[1] = g;
        m_clearColor[2] = b;
        m_clearColor[3] = a;
    }
}

void StateCacheGL::SetClearDepth(float depth)
{
    if (!m_valid || m_clearDepth != depth) {
        glClearDepth(depth);
        m_clearDepth = depth;
    }
}

void StateCacheGL::SetClearStencil(int32_t stencil)
{
    if (!m_valid || m_clearStencil != stencil) {
        glClearStencil(stencil);
        m_clearStencil = stencil;
    }
}

} // namespace gl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGL
