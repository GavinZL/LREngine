/**
 * @file RenderStateStackMTL.mm
 * @brief Metal渲染状态栈实现
 */

#include "RenderStateStackMTL.h"
#include "lrengine/utils/LRLog.h"

#ifdef LRENGINE_ENABLE_METAL

namespace lrengine {
namespace render {
namespace mtl {

RenderStateStackMTL::RenderStateStackMTL() {
}

RenderStateStackMTL::~RenderStateStackMTL() {
    Clear();
}

void RenderStateStackMTL::PushState(const RenderState& state) {
    RenderState newState = state;
    newState.depth = static_cast<uint32_t>(m_stateStack.size());
    
    m_stateStack.push_back(newState);
    
    LR_LOG_INFO_F("RenderStateStackMTL: Pushed state (depth: %u)", newState.depth);
}

RenderState RenderStateStackMTL::PopState() {
    if (m_stateStack.empty()) {
        LR_LOG_ERROR_F("RenderStateStackMTL: Cannot pop from empty stack");
        return RenderState();
    }
    
    RenderState state = m_stateStack.back();
    m_stateStack.pop_back();
    
    LR_LOG_INFO_F("RenderStateStackMTL: Popped state (depth was: %u, now: %zu)",
                state.depth, m_stateStack.size());
    
    return state;
}

RenderState& RenderStateStackMTL::GetCurrentState() {
    if (m_stateStack.empty()) {
        LR_LOG_ERROR_F("RenderStateStackMTL: Stack is empty, cannot get current state");
        static RenderState emptyState;
        return emptyState;
    }
    
    return m_stateStack.back();
}

const RenderState& RenderStateStackMTL::GetCurrentState() const {
    if (m_stateStack.empty()) {
        LR_LOG_ERROR_F("RenderStateStackMTL: Stack is empty, cannot get current state");
        static RenderState emptyState;
        return emptyState;
    }
    
    return m_stateStack.back();
}

void RenderStateStackMTL::Clear() {
    if (!m_stateStack.empty()) {
        LR_LOG_WARNING_F("RenderStateStackMTL: Clearing non-empty stack (depth: %zu)",
                       m_stateStack.size());
    }
    
    m_stateStack.clear();
}

} // namespace mtl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
