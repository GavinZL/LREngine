/**
 * @file PipelineStateMTL.mm
 * @brief Metal管线状态实现
 */

#include "PipelineStateMTL.h"
#include "ShaderMTL.h"

#ifdef LRENGINE_ENABLE_METAL

#include "TypeConverterMTL.h"
#include "lrengine/core/LRError.h"
#include "lrengine/core/LRShader.h"

namespace lrengine {
namespace render {
namespace mtl {

PipelineStateMTL::PipelineStateMTL(id<MTLDevice> device)
    : m_device(device)
    , m_pipelineState(nil)
    , m_depthStencilState(nil)
    , m_primitiveType(PrimitiveType::Triangles)
    , m_cullMode(MTLCullModeBack)
    , m_frontFace(MTLWindingCounterClockwise)
    , m_fillMode(MTLTriangleFillModeFill)
    , m_depthBias(0.0f)
    , m_depthBiasSlopeFactor(0.0f)
    , m_depthBiasClamp(0.0f)
    , m_depthBiasEnabled(false)
    , m_scissorEnabled(false)
{
}

PipelineStateMTL::~PipelineStateMTL() {
    Destroy();
}

bool PipelineStateMTL::Create(const PipelineStateDescriptor& desc) {
    m_primitiveType = desc.primitiveType;

    // 创建渲染管线描述符
    MTLRenderPipelineDescriptor* pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];

    // 设置着色器函数
    if (desc.vertexShader) {
        IShaderImpl* vsImpl = desc.vertexShader->GetImpl();
        if (vsImpl) {
            ShaderMTL* vsMTL = static_cast<ShaderMTL*>(vsImpl);
            pipelineDesc.vertexFunction = vsMTL->GetFunction();
        }
    }

    if (desc.fragmentShader) {
        IShaderImpl* fsImpl = desc.fragmentShader->GetImpl();
        if (fsImpl) {
            ShaderMTL* fsMTL = static_cast<ShaderMTL*>(fsImpl);
            pipelineDesc.fragmentFunction = fsMTL->GetFunction();
        }
    }

    if (!pipelineDesc.vertexFunction) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Vertex shader is required for pipeline state");
        return false;
    }

    // 设置顶点描述符
    if (!desc.vertexLayout.attributes.empty()) {
        MTLVertexDescriptor* vertexDesc = [[MTLVertexDescriptor alloc] init];
        
        for (const auto& attr : desc.vertexLayout.attributes) {
            vertexDesc.attributes[attr.location].format = ToMTLVertexFormat(attr.format);
            vertexDesc.attributes[attr.location].offset = attr.offset;
            vertexDesc.attributes[attr.location].bufferIndex = 0; // 默认使用buffer 0
        }
        
        vertexDesc.layouts[0].stride = desc.vertexLayout.stride;
        vertexDesc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
        vertexDesc.layouts[0].stepRate = 1;
        
        pipelineDesc.vertexDescriptor = vertexDesc;
    }

    // 设置颜色附件
    pipelineDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    
    // 设置混合状态
    if (desc.blendState.enabled) {
        pipelineDesc.colorAttachments[0].blendingEnabled = YES;
        pipelineDesc.colorAttachments[0].sourceRGBBlendFactor = ToMTLBlendFactor(desc.blendState.srcColorFactor);
        pipelineDesc.colorAttachments[0].destinationRGBBlendFactor = ToMTLBlendFactor(desc.blendState.dstColorFactor);
        pipelineDesc.colorAttachments[0].rgbBlendOperation = ToMTLBlendOperation(desc.blendState.colorOp);
        pipelineDesc.colorAttachments[0].sourceAlphaBlendFactor = ToMTLBlendFactor(desc.blendState.srcAlphaFactor);
        pipelineDesc.colorAttachments[0].destinationAlphaBlendFactor = ToMTLBlendFactor(desc.blendState.dstAlphaFactor);
        pipelineDesc.colorAttachments[0].alphaBlendOperation = ToMTLBlendOperation(desc.blendState.alphaOp);
    }
    
    // 设置颜色写入掩码
    MTLColorWriteMask writeMask = MTLColorWriteMaskNone;
    if (desc.blendState.colorWriteMask & 0x01) writeMask |= MTLColorWriteMaskRed;
    if (desc.blendState.colorWriteMask & 0x02) writeMask |= MTLColorWriteMaskGreen;
    if (desc.blendState.colorWriteMask & 0x04) writeMask |= MTLColorWriteMaskBlue;
    if (desc.blendState.colorWriteMask & 0x08) writeMask |= MTLColorWriteMaskAlpha;
    pipelineDesc.colorAttachments[0].writeMask = writeMask;

    // 设置深度格式
    pipelineDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
    
    // 设置采样数（使用新API rasterSampleCount）
    if (@available(macOS 13.0, iOS 16.0, *)) {
        pipelineDesc.rasterSampleCount = desc.sampleCount;
    } else {
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wdeprecated-declarations"
        pipelineDesc.sampleCount = desc.sampleCount;
        #pragma clang diagnostic pop
    }

    if (desc.debugName) {
        pipelineDesc.label = [NSString stringWithUTF8String:desc.debugName];
    }

    // 创建管线状态
    NSError* error = nil;
    m_pipelineState = [m_device newRenderPipelineStateWithDescriptor:pipelineDesc
                                                               error:&error];
    
    if (error || !m_pipelineState) {
        std::string errorMsg = "Failed to create Metal render pipeline state";
        if (error) {
            errorMsg += ": ";
            errorMsg += [[error localizedDescription] UTF8String];
        }
        LR_SET_ERROR(ErrorCode::ResourceCreationFailed, errorMsg.c_str());
        return false;
    }

    // 创建深度模板状态
    MTLDepthStencilDescriptor* depthStencilDesc = [[MTLDepthStencilDescriptor alloc] init];
    
    depthStencilDesc.depthCompareFunction = desc.depthStencilState.depthTestEnabled
        ? ToMTLCompareFunction(desc.depthStencilState.depthCompareFunc)
        : MTLCompareFunctionAlways;
    depthStencilDesc.depthWriteEnabled = desc.depthStencilState.depthWriteEnabled;
    
    // 设置模板状态
    if (desc.depthStencilState.stencilEnabled) {
        MTLStencilDescriptor* frontStencil = [[MTLStencilDescriptor alloc] init];
        frontStencil.stencilCompareFunction = ToMTLCompareFunction(desc.depthStencilState.frontFace.compareFunc);
        frontStencil.stencilFailureOperation = ToMTLStencilOperation(desc.depthStencilState.frontFace.failOp);
        frontStencil.depthFailureOperation = ToMTLStencilOperation(desc.depthStencilState.frontFace.depthFailOp);
        frontStencil.depthStencilPassOperation = ToMTLStencilOperation(desc.depthStencilState.frontFace.passOp);
        frontStencil.readMask = desc.depthStencilState.stencilReadMask;
        frontStencil.writeMask = desc.depthStencilState.stencilWriteMask;
        depthStencilDesc.frontFaceStencil = frontStencil;
        
        MTLStencilDescriptor* backStencil = [[MTLStencilDescriptor alloc] init];
        backStencil.stencilCompareFunction = ToMTLCompareFunction(desc.depthStencilState.backFace.compareFunc);
        backStencil.stencilFailureOperation = ToMTLStencilOperation(desc.depthStencilState.backFace.failOp);
        backStencil.depthFailureOperation = ToMTLStencilOperation(desc.depthStencilState.backFace.depthFailOp);
        backStencil.depthStencilPassOperation = ToMTLStencilOperation(desc.depthStencilState.backFace.passOp);
        backStencil.readMask = desc.depthStencilState.stencilReadMask;
        backStencil.writeMask = desc.depthStencilState.stencilWriteMask;
        depthStencilDesc.backFaceStencil = backStencil;
    }
    
    m_depthStencilState = [m_device newDepthStencilStateWithDescriptor:depthStencilDesc];

    // 保存光栅化状态（这些需要在编码时设置）
    m_cullMode = ToMTLCullMode(desc.rasterizerState.cullMode);
    m_frontFace = ToMTLWinding(desc.rasterizerState.frontFace);
    m_fillMode = ToMTLTriangleFillMode(desc.rasterizerState.fillMode);
    m_depthBias = desc.rasterizerState.depthBias;
    m_depthBiasSlopeFactor = desc.rasterizerState.depthBiasSlopeFactor;
    m_depthBiasClamp = desc.rasterizerState.depthBiasClamp;
    m_depthBiasEnabled = desc.rasterizerState.depthBiasEnabled;
    m_scissorEnabled = desc.rasterizerState.scissorEnabled;

    return true;
}

void PipelineStateMTL::Destroy() {
    m_pipelineState = nil;
    m_depthStencilState = nil;
}

void PipelineStateMTL::Apply() {
    // Metal中管线状态在渲染命令编码器中设置
    // 这个方法主要用于标记当前管线状态为活动状态
}

ResourceHandle PipelineStateMTL::GetNativeHandle() const {
    return ResourceHandle((__bridge void*)m_pipelineState);
}

PrimitiveType PipelineStateMTL::GetPrimitiveType() const {
    return m_primitiveType;
}

} // namespace mtl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
