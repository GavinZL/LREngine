/**
 * @file LRPipelineState.h
 * @brief LREngine管线状态对象
 */

#pragma once

#include "LRResource.h"
#include "LRTypes.h"

namespace lrengine {
namespace render {

class IPipelineStateImpl;
class LRShaderProgram;

/**
 * @brief 管线状态类
 * 
 * 封装完整的图形管线配置，包括：
 * - 着色器程序
 * - 顶点布局
 * - 混合状态
 * - 深度模板状态
 * - 光栅化状态
 * 
 * 管线状态对象是不可变的，创建后不能修改
 */
class LR_API LRPipelineState : public LRResource {
public:
    LR_NONCOPYABLE(LRPipelineState);
    
    virtual ~LRPipelineState();
    
    /**
     * @brief 应用管线状态
     */
    void Apply();
    
    /**
     * @brief 获取着色器程序
     */
    LRShaderProgram* GetShaderProgram() const { return mShaderProgram; }
    
    /**
     * @brief 获取顶点布局
     */
    const VertexLayoutDescriptor& GetVertexLayout() const { return mVertexLayout; }
    
    /**
     * @brief 获取混合状态
     */
    const BlendStateDescriptor& GetBlendState() const { return mBlendState; }
    
    /**
     * @brief 获取深度模板状态
     */
    const DepthStencilStateDescriptor& GetDepthStencilState() const { return mDepthStencilState; }
    
    /**
     * @brief 获取光栅化状态
     */
    const RasterizerStateDescriptor& GetRasterizerState() const { return mRasterizerState; }
    
    /**
     * @brief 获取图元类型
     */
    PrimitiveType GetPrimitiveType() const { return mPrimitiveType; }
    
    /**
     * @brief 获取原生句柄
     */
    ResourceHandle GetNativeHandle() const override;
    
    /**
     * @brief 获取平台实现
     */
    IPipelineStateImpl* GetImpl() const { return mImpl; }
    
protected:
    friend class LRRenderContext;
    
    LRPipelineState();
    bool Initialize(IPipelineStateImpl* impl, const PipelineStateDescriptor& desc, LRShaderProgram* program);
    
protected:
    IPipelineStateImpl* mImpl = nullptr;
    LRShaderProgram* mShaderProgram = nullptr;
    VertexLayoutDescriptor mVertexLayout;
    BlendStateDescriptor mBlendState;
    DepthStencilStateDescriptor mDepthStencilState;
    RasterizerStateDescriptor mRasterizerState;
    PrimitiveType mPrimitiveType = PrimitiveType::Triangles;
};

} // namespace render
} // namespace lrengine
