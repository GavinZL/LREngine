/**
 * @file RenderStateStackMTL.h
 * @brief Metal渲染状态栈
 * 
 * 支持嵌套的RenderPass（例如：阴影贴图 -> 主渲染 -> 后处理）
 */

#pragma once

#include <vector>
#include <cstdint>

#ifdef LRENGINE_ENABLE_METAL

#import <Metal/Metal.h>

namespace lrengine {
namespace render {
namespace mtl {

class RenderPassMTL;
class PipelineStateMTL;

/**
 * @brief 渲染状态
 */
struct RenderState {
    // 当前渲染通道
    RenderPassMTL* renderPass = nullptr;

    // Metal对象
    id<MTLRenderCommandEncoder> encoder = nil;
    id<MTLCommandBuffer> commandBuffer  = nil;

    // 视口和裁剪
    MTLViewport viewport;
    MTLScissorRect scissor;

    // 管线状态
    PipelineStateMTL* pipelineState = nullptr;

    // 深度
    uint32_t depth = 0; // 栈深度，用于调试
};

/**
 * @brief 渲染状态栈
 * 
 * 功能：
 * 1. 支持嵌套的RenderPass
 * 2. 自动保存/恢复视口等状态
 * 3. 提供清晰的作用域管理
 */
class RenderStateStackMTL {
public:
    RenderStateStackMTL();
    ~RenderStateStackMTL();

    /**
     * @brief 压入新状态
     */
    void PushState(const RenderState& state);

    /**
     * @brief 弹出当前状态
     * @return 弹出的状态
     */
    RenderState PopState();

    /**
     * @brief 获取当前状态（栈顶）
     */
    RenderState& GetCurrentState();
    const RenderState& GetCurrentState() const;

    /**
     * @brief 检查栈是否为空
     */
    bool IsEmpty() const { return m_stateStack.empty(); }

    /**
     * @brief 获取当前栈深度
     */
    uint32_t GetDepth() const { return static_cast<uint32_t>(m_stateStack.size()); }

    /**
     * @brief 清空栈
     */
    void Clear();

private:
    std::vector<RenderState> m_stateStack;
};

} // namespace mtl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
