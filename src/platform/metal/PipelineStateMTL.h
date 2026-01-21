/**
 * @file PipelineStateMTL.h
 * @brief Metal管线状态实现
 */

#pragma once

#include "platform/interface/IPipelineStateImpl.h"

#ifdef LRENGINE_ENABLE_METAL

#import <Metal/Metal.h>

namespace lrengine {
namespace render {
namespace mtl {

/**
 * @brief Metal管线状态实现
 */
class PipelineStateMTL : public IPipelineStateImpl {
public:
    PipelineStateMTL(id<MTLDevice> device);
    ~PipelineStateMTL() override;

    // IPipelineStateImpl接口
    bool Create(const PipelineStateDescriptor& desc) override;
    void Destroy() override;
    void Apply() override;
    ResourceHandle GetNativeHandle() const override;
    PrimitiveType GetPrimitiveType() const override;

    // Metal特有方法
    id<MTLRenderPipelineState> GetPipelineState() const { return m_pipelineState; }
    id<MTLDepthStencilState> GetDepthStencilState() const { return m_depthStencilState; }

    MTLCullMode GetCullMode() const { return m_cullMode; }
    MTLWinding GetFrontFace() const { return m_frontFace; }
    MTLTriangleFillMode GetFillMode() const { return m_fillMode; }
    float GetDepthBias() const { return m_depthBias; }
    float GetDepthBiasSlopeFactor() const { return m_depthBiasSlopeFactor; }
    float GetDepthBiasClamp() const { return m_depthBiasClamp; }

private:
    id<MTLDevice> m_device;
    id<MTLRenderPipelineState> m_pipelineState;
    id<MTLDepthStencilState> m_depthStencilState;

    PrimitiveType m_primitiveType;

    // 光栅化状态（需要在编码时设置）
    MTLCullMode m_cullMode;
    MTLWinding m_frontFace;
    MTLTriangleFillMode m_fillMode;
    float m_depthBias;
    float m_depthBiasSlopeFactor;
    float m_depthBiasClamp;
    bool m_depthBiasEnabled;
    bool m_scissorEnabled;
};

} // namespace mtl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
