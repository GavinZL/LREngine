/**
 * @file LRPipelineState.cpp
 * @brief LREngine管线状态实现
 */

#include "lrengine/core/LRPipelineState.h"
#include "lrengine/core/LRShader.h"
#include "lrengine/core/LRError.h"
#include "platform/interface/IPipelineStateImpl.h"

namespace lrengine {
namespace render {

LRPipelineState::LRPipelineState() : LRResource(ResourceType::PipelineState) {}

LRPipelineState::~LRPipelineState() {
    if (mImpl) {
        mImpl->Destroy();
        delete mImpl;
        mImpl = nullptr;
    }
}

bool LRPipelineState::Initialize(IPipelineStateImpl* impl,
                                 const PipelineStateDescriptor& desc,
                                 LRShaderProgram* program) {
    if (!impl) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "PipelineState implementation is null");
        return false;
    }

    mImpl              = impl;
    mShaderProgram     = program;
    mVertexLayout      = desc.vertexLayout;
    mBlendState        = desc.blendState;
    mDepthStencilState = desc.depthStencilState;
    mRasterizerState   = desc.rasterizerState;
    mPrimitiveType     = desc.primitiveType;

    if (!mImpl->Create(desc)) {
        LR_SET_ERROR(ErrorCode::PipelineCreationFailed, "Failed to create pipeline state");
        delete mImpl;
        mImpl = nullptr;
        return false;
    }

    mIsValid = true;

    if (desc.debugName) {
        SetDebugName(desc.debugName);
    }

    return true;
}

void LRPipelineState::Apply() {
    if (mImpl && mIsValid) {
        mImpl->Apply();
    }
}

ResourceHandle LRPipelineState::GetNativeHandle() const {
    if (mImpl) {
        return mImpl->GetNativeHandle();
    }
    return ResourceHandle();
}

} // namespace render
} // namespace lrengine
