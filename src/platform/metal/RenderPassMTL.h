/**
 * @file RenderPassMTL.h
 * @brief Metal渲染通道抽象
 * 
 * 封装MTLRenderPassDescriptor的创建和管理，支持：
 * - 多渲染目标(MRT)
 * - 灵活的LoadAction/StoreAction配置
 * - 线程安全的Descriptor创建
 */

#pragma once

#include <vector>
#include <memory>
#include <cstdint>

#ifdef LRENGINE_ENABLE_METAL

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

namespace lrengine {
namespace render {
namespace mtl {

class FrameBufferMTL;

/**
 * @brief 渲染通道配置
 * 
 * 设计说明：
 * - 使用单个FrameBuffer管理所有颜色附件（MRT）
 * - 符合OpenGL/Vulkan的标准FrameBuffer设计
 * - 深度附件从同一个FrameBuffer获取
 */
struct RenderPassConfig {
    // 颜色和深度附件的FrameBuffer（支持多渲染目标MRT）
    FrameBufferMTL* frameBuffer = nullptr;

    // LoadAction配置
    MTLLoadAction colorLoadAction = MTLLoadActionClear;
    MTLLoadAction depthLoadAction = MTLLoadActionClear;

    // StoreAction配置
    MTLStoreAction colorStoreAction = MTLStoreActionStore;
    MTLStoreAction depthStoreAction = MTLStoreActionDontCare;

    // 清除值
    float clearColor[4]  = {0.0f, 0.0f, 0.0f, 1.0f};
    float clearDepth     = 1.0f;
    uint8_t clearStencil = 0;

    // 是否使用默认帧缓冲（屏幕）
    bool useDefaultFrameBuffer = false;

    // 默认帧缓冲的drawable（当useDefaultFrameBuffer=true时使用）
    id<CAMetalDrawable> drawable = nil;

    // 默认帧缓冲的深度纹理
    id<MTLTexture> defaultDepthTexture = nil;
};

/**
 * @brief Metal渲染通道抽象
 * 
 * 职责：
 * 1. 封装RenderPassDescriptor的创建逻辑
 * 2. 管理多渲染目标配置
 * 3. 提供线程安全的Descriptor生成
 */
class RenderPassMTL {
public:
    explicit RenderPassMTL(const RenderPassConfig& config);
    ~RenderPassMTL();

    /**
     * @brief 创建MTLRenderPassDescriptor
     * 
     * 注意：每次调用都返回新的实例，线程安全
     * 调用者不需要手动释放（ARC管理）
     */
    MTLRenderPassDescriptor* CreateDescriptor() const;

    /**
     * @brief 获取渲染目标尺寸
     */
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }

    /**
     * @brief 获取颜色目标数量
     */
    uint32_t GetColorTargetCount() const;

    /**
     * @brief 是否使用默认帧缓冲
     */
    bool IsDefaultFrameBuffer() const { return m_config.useDefaultFrameBuffer; }

    /**
     * @brief 获取配置
     */
    const RenderPassConfig& GetConfig() const { return m_config; }

    /**
     * @brief 更新清除值（用于动态修改）
     */
    void SetClearColor(float r, float g, float b, float a);
    void SetClearDepth(float depth);
    void SetClearStencil(uint8_t stencil);

private:
    void CalculateDimensions();

    RenderPassConfig m_config;
    uint32_t m_width;
    uint32_t m_height;
};

} // namespace mtl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
