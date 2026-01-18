/**
 * @file PipelineStateGLES.cpp
 * @brief OpenGL ES管线状态实现
 */

#include "PipelineStateGLES.h"
#include "lrengine/core/LRError.h"
#include "lrengine/utils/LRLog.h"

#ifdef LRENGINE_ENABLE_OPENGLES

namespace lrengine {
namespace render {
namespace gles {

PipelineStateGLES::PipelineStateGLES()
    : m_valid(false)
{
}

PipelineStateGLES::~PipelineStateGLES()
{
    Destroy();
}

bool PipelineStateGLES::Create(const PipelineStateDescriptor& desc)
{
    m_desc = desc;
    m_valid = true;
    return true;
}

void PipelineStateGLES::Destroy()
{
    m_valid = false;
}

void PipelineStateGLES::Apply()
{
    if (!m_valid) {
        return;
    }

    ApplyBlendState();
    ApplyDepthStencilState();
    ApplyRasterizerState();
}

void PipelineStateGLES::ApplyBlendState()
{
    const BlendStateDescriptor& blend = m_desc.blendState;

    if (blend.enabled) {
        glEnable(GL_BLEND);
        glBlendFuncSeparate(
            ToGLESBlendFactor(blend.srcColorFactor),
            ToGLESBlendFactor(blend.dstColorFactor),
            ToGLESBlendFactor(blend.srcAlphaFactor),
            ToGLESBlendFactor(blend.dstAlphaFactor)
        );
        glBlendEquationSeparate(
            ToGLESBlendOp(blend.colorOp),
            ToGLESBlendOp(blend.alphaOp)
        );
    } else {
        glDisable(GL_BLEND);
    }

    // 颜色写入掩码
    glColorMask(
        (blend.colorWriteMask & ColorMask_R) ? GL_TRUE : GL_FALSE,
        (blend.colorWriteMask & ColorMask_G) ? GL_TRUE : GL_FALSE,
        (blend.colorWriteMask & ColorMask_B) ? GL_TRUE : GL_FALSE,
        (blend.colorWriteMask & ColorMask_A) ? GL_TRUE : GL_FALSE
    );
}

void PipelineStateGLES::ApplyDepthStencilState()
{
    const DepthStencilStateDescriptor& ds = m_desc.depthStencilState;

    // 重要：先设置深度写入，再设置深度测试
    // 因为 glClear(GL_DEPTH_BUFFER_BIT) 需要 glDepthMask(GL_TRUE) 才能生效
    
    // 1. 深度写入
    glDepthMask(ds.depthWriteEnabled ? GL_TRUE : GL_FALSE);
    LR_LOG_TRACE_F("[PipelineStateGLES] Depth Write: %s", 
                   ds.depthWriteEnabled ? "ENABLED" : "DISABLED");

    // 2. 深度测试
    if (ds.depthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(ToGLESCompareFunc(ds.depthCompareFunc));
        LR_LOG_TRACE_F("[PipelineStateGLES] Depth Test ENABLED (func=%d)", 
                       (int)ds.depthCompareFunc);
    } else {
        glDisable(GL_DEPTH_TEST);
        LR_LOG_TRACE("[PipelineStateGLES] Depth Test DISABLED");
    }

    // 模板测试
    if (ds.stencilEnabled) {
        glEnable(GL_STENCIL_TEST);
        
        // 前面模板
        glStencilFuncSeparate(
            GL_FRONT,
            ToGLESCompareFunc(ds.frontFace.compareFunc),
            ds.stencilRef,
            ds.stencilReadMask
        );
        glStencilOpSeparate(
            GL_FRONT,
            ToGLESStencilOp(ds.frontFace.failOp),
            ToGLESStencilOp(ds.frontFace.depthFailOp),
            ToGLESStencilOp(ds.frontFace.passOp)
        );

        // 背面模板
        glStencilFuncSeparate(
            GL_BACK,
            ToGLESCompareFunc(ds.backFace.compareFunc),
            ds.stencilRef,
            ds.stencilReadMask
        );
        glStencilOpSeparate(
            GL_BACK,
            ToGLESStencilOp(ds.backFace.failOp),
            ToGLESStencilOp(ds.backFace.depthFailOp),
            ToGLESStencilOp(ds.backFace.passOp)
        );

        glStencilMask(ds.stencilWriteMask);
    } else {
        glDisable(GL_STENCIL_TEST);
    }
}

void PipelineStateGLES::ApplyRasterizerState()
{
    const RasterizerStateDescriptor& raster = m_desc.rasterizerState;

    // 面剔除
    if (raster.cullMode != CullMode::None) {
        glEnable(GL_CULL_FACE);
        glCullFace(ToGLESCullMode(raster.cullMode));
    } else {
        glDisable(GL_CULL_FACE);
    }

    // 正面方向
    glFrontFace(ToGLESFrontFace(raster.frontFace));

    // 填充模式
    // 注意：OpenGL ES不支持glPolygonMode，只能使用填充模式
    if (raster.fillMode != FillMode::Solid) {
        // 发出警告，但继续执行
        LR_LOG_WARNING("OpenGL ES does not support wireframe/point mode, using solid fill");
    }

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
        LR_LOG_TRACE("[PipelineStateGLES] Scissor Test ENABLED");
    } else {
        glDisable(GL_SCISSOR_TEST);
        LR_LOG_TRACE("[PipelineStateGLES] Scissor Test DISABLED");
    }

    // 多采样
    // 注意：OpenGL ES中多采样是通过渲染目标配置的，而不是glEnable
    // 但某些实现可能支持GL_SAMPLE_ALPHA_TO_COVERAGE
    if (raster.multisampleEnabled) {
#ifdef GL_SAMPLE_ALPHA_TO_COVERAGE
        glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
#endif
    } else {
#ifdef GL_SAMPLE_ALPHA_TO_COVERAGE
        glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
#endif
    }

    // 线宽
    glLineWidth(raster.lineWidth);
}

PrimitiveType PipelineStateGLES::GetPrimitiveType() const
{
    return m_desc.primitiveType;
}

ResourceHandle PipelineStateGLES::GetNativeHandle() const
{
    ResourceHandle handle;
    handle.glHandle = 0; // OpenGL ES没有管线状态对象
    return handle;
}

} // namespace gles
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGLES
