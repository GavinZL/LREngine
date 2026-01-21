/**
 * @file PipelineStateGL.cpp
 * @brief OpenGL管线状态实现
 */

#include "PipelineStateGL.h"
#include "lrengine/core/LRError.h"
#include "lrengine/utils/LRLog.h"

#ifdef LRENGINE_ENABLE_OPENGL

namespace lrengine {
namespace render {
namespace gl {

PipelineStateGL::PipelineStateGL() : m_valid(false) {}

PipelineStateGL::~PipelineStateGL() { Destroy(); }

bool PipelineStateGL::Create(const PipelineStateDescriptor& desc) {
    m_desc  = desc;
    m_valid = true;
    return true;
}

void PipelineStateGL::Destroy() { m_valid = false; }

void PipelineStateGL::Apply() {
    if (!m_valid) {
        return;
    }

    ApplyBlendState();
    ApplyDepthStencilState();
    ApplyRasterizerState();
}

void PipelineStateGL::ApplyBlendState() {
    const BlendStateDescriptor& blend = m_desc.blendState;

    if (blend.enabled) {
        glEnable(GL_BLEND);
        glBlendFuncSeparate(ToGLBlendFactor(blend.srcColorFactor),
                            ToGLBlendFactor(blend.dstColorFactor),
                            ToGLBlendFactor(blend.srcAlphaFactor),
                            ToGLBlendFactor(blend.dstAlphaFactor));
        glBlendEquationSeparate(ToGLBlendOp(blend.colorOp), ToGLBlendOp(blend.alphaOp));
    } else {
        glDisable(GL_BLEND);
    }

    // 颜色写入掩码
    glColorMask((blend.colorWriteMask & ColorMask_R) ? GL_TRUE : GL_FALSE,
                (blend.colorWriteMask & ColorMask_G) ? GL_TRUE : GL_FALSE,
                (blend.colorWriteMask & ColorMask_B) ? GL_TRUE : GL_FALSE,
                (blend.colorWriteMask & ColorMask_A) ? GL_TRUE : GL_FALSE);
}

void PipelineStateGL::ApplyDepthStencilState() {
    const DepthStencilStateDescriptor& ds = m_desc.depthStencilState;

    // ❗重要：先设置深度写入，再设置深度测试
    // 因为 glClear(GL_DEPTH_BUFFER_BIT) 需要 glDepthMask(GL_TRUE) 才能生效
    // 如果先禁用深度测试，再关闭深度写入，会导致下一帧的 Clear 无法清除深度缓冲

    // 1. 深度写入（必须在深度测试之前设置）
    glDepthMask(ds.depthWriteEnabled ? GL_TRUE : GL_FALSE);
    LR_LOG_TRACE_F("[PipelineState] Depth Write: %s", ds.depthWriteEnabled ? "ENABLED" : "DISABLED");

    // 2. 深度测试
    if (ds.depthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(ToGLCompareFunc(ds.depthCompareFunc));
        LR_LOG_TRACE_F("[PipelineState] Depth Test ENABLED (func=%d)", (int)ds.depthCompareFunc);
    } else {
        glDisable(GL_DEPTH_TEST);
        LR_LOG_TRACE("[PipelineState] Depth Test DISABLED");
    }

    // 模板测试
    if (ds.stencilEnabled) {
        glEnable(GL_STENCIL_TEST);

        // 前面模板
        glStencilFuncSeparate(GL_FRONT, ToGLCompareFunc(ds.frontFace.compareFunc), ds.stencilRef,
                              ds.stencilReadMask);
        glStencilOpSeparate(GL_FRONT, ToGLStencilOp(ds.frontFace.failOp),
                            ToGLStencilOp(ds.frontFace.depthFailOp),
                            ToGLStencilOp(ds.frontFace.passOp));

        // 背面模板
        glStencilFuncSeparate(GL_BACK, ToGLCompareFunc(ds.backFace.compareFunc), ds.stencilRef,
                              ds.stencilReadMask);
        glStencilOpSeparate(GL_BACK, ToGLStencilOp(ds.backFace.failOp),
                            ToGLStencilOp(ds.backFace.depthFailOp),
                            ToGLStencilOp(ds.backFace.passOp));

        glStencilMask(ds.stencilWriteMask);
    } else {
        glDisable(GL_STENCIL_TEST);
    }
}

void PipelineStateGL::ApplyRasterizerState() {
    const RasterizerStateDescriptor& raster = m_desc.rasterizerState;

    // 面剔除
    if (raster.cullMode != CullMode::None) {
        glEnable(GL_CULL_FACE);
        glCullFace(ToGLCullMode(raster.cullMode));
    } else {
        glDisable(GL_CULL_FACE);
    }

    // 正面方向
    glFrontFace(ToGLFrontFace(raster.frontFace));

    // 填充模式
    GLenum fillMode = GL_FILL;
    switch (raster.fillMode) {
        case FillMode::Solid:
            fillMode = GL_FILL;
            break;
        case FillMode::Wireframe:
            fillMode = GL_LINE;
            break;
        case FillMode::Point:
            fillMode = GL_POINT;
            break;
    }
    glPolygonMode(GL_FRONT_AND_BACK, fillMode);

    // 深度偏移
    if (raster.depthBiasEnabled) {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(raster.depthBiasSlopeFactor, static_cast<float>(raster.depthBias));
    } else {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    // 裁剪测试
    if (raster.scissorEnabled) {
        glEnable(GL_SCISSOR_TEST);
        LR_LOG_TRACE("[PipelineState] Scissor Test ENABLED");
    } else {
        glDisable(GL_SCISSOR_TEST);
        LR_LOG_TRACE("[PipelineState] Scissor Test DISABLED");
    }

    // 多采样
    if (raster.multisampleEnabled) {
        glEnable(GL_MULTISAMPLE);
    } else {
        glDisable(GL_MULTISAMPLE);
    }

    // 线宽
    glLineWidth(raster.lineWidth);
}

PrimitiveType PipelineStateGL::GetPrimitiveType() const { return m_desc.primitiveType; }

ResourceHandle PipelineStateGL::GetNativeHandle() const {
    ResourceHandle handle;
    handle.glHandle = 0; // OpenGL没有管线状态对象
    return handle;
}

} // namespace gl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGL
