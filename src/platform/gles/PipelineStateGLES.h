/**
 * @file PipelineStateGLES.h
 * @brief OpenGL ES管线状态实现
 */

#pragma once

#include "platform/interface/IPipelineStateImpl.h"
#include "TypeConverterGLES.h"

#ifdef LRENGINE_ENABLE_OPENGLES

namespace lrengine {
namespace render {
namespace gles {

/**
 * @brief OpenGL ES管线状态实现
 * 
 * 注意：OpenGL ES不支持某些桌面OpenGL的功能，如：
 * - glPolygonMode（线框模式）
 */
class PipelineStateGLES : public IPipelineStateImpl {
public:
    PipelineStateGLES();
    ~PipelineStateGLES() override;

    // IPipelineStateImpl接口
    bool Create(const PipelineStateDescriptor& desc) override;
    void Destroy() override;
    void Apply() override;
    PrimitiveType GetPrimitiveType() const override;
    ResourceHandle GetNativeHandle() const override;

private:
    void ApplyBlendState();
    void ApplyDepthStencilState();
    void ApplyRasterizerState();

    PipelineStateDescriptor m_desc;
    bool m_valid;
};

} // namespace gles
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGLES
