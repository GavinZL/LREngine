/**
 * @file PipelineStateGL.h
 * @brief OpenGL管线状态实现
 */

#pragma once

#include "platform/interface/IPipelineStateImpl.h"
#include "TypeConverterGL.h"

#ifdef LRENGINE_ENABLE_OPENGL

namespace lrengine {
namespace render {
namespace gl {

/**
 * @brief OpenGL管线状态实现
 * 
 * OpenGL没有管线状态对象，此类用于管理和应用各种渲染状态
 */
class PipelineStateGL : public IPipelineStateImpl {
public:
    PipelineStateGL();
    ~PipelineStateGL() override;

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

} // namespace gl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGL
