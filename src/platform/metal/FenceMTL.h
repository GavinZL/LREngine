/**
 * @file FenceMTL.h
 * @brief Metal同步栅栏实现
 */

#pragma once

#include "platform/interface/IFenceImpl.h"

#ifdef LRENGINE_ENABLE_METAL

#import <Metal/Metal.h>

namespace lrengine {
namespace render {
namespace mtl {

/**
 * @brief Metal同步栅栏实现
 * 
 * 使用MTLSharedEvent实现GPU-CPU同步
 */
class FenceMTL : public IFenceImpl {
public:
    FenceMTL(id<MTLDevice> device);
    ~FenceMTL() override;

    // IFenceImpl接口
    bool Create() override;
    void Destroy() override;
    void Signal() override;
    bool Wait(uint64_t timeoutNs) override;
    FenceStatus GetStatus() const override;
    void Reset() override;
    ResourceHandle GetNativeHandle() const override;

    // Metal特有方法
    id<MTLSharedEvent> GetEvent() const { return m_event; }
    uint64_t GetSignaledValue() const { return m_signaledValue; }
    
    // 用于命令缓冲区编码
    void SignalOnCommandBuffer(id<MTLCommandBuffer> commandBuffer);

private:
    id<MTLDevice> m_device;
    id<MTLSharedEvent> m_event;
    MTLSharedEventListener* m_listener;
    dispatch_queue_t m_listenerQueue;
    
    uint64_t m_signaledValue;
    uint64_t m_currentValue;
    bool m_signaled;
};

} // namespace mtl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
