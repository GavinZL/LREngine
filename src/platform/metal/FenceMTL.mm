/**
 * @file FenceMTL.mm
 * @brief Metal同步栅栏实现
 */

#include "FenceMTL.h"

#ifdef LRENGINE_ENABLE_METAL

#include "lrengine/core/LRError.h"
#include <chrono>

namespace lrengine {
namespace render {
namespace mtl {

FenceMTL::FenceMTL(id<MTLDevice> device)
    : m_device(device)
    , m_event(nil)
    , m_listener(nil)
    , m_listenerQueue(nil)
    , m_signaledValue(0)
    , m_currentValue(0)
    , m_signaled(false)
{
}

FenceMTL::~FenceMTL() {
    Destroy();
}

bool FenceMTL::Create() {
    // 检查设备是否支持共享事件
    // MTLGPUFamilyApple1在iOS 8.0+和macOS 10.14+上可用
    #if TARGET_OS_IPHONE || TARGET_OS_IOS
        // iOS使用GPU Family检测
        if (@available(iOS 8.0, *)) {
            if (![m_device supportsFamily:MTLGPUFamilyApple1]) {
                // 对于不支持共享事件的旧设备，使用简化实现
                m_signaled = false;
                return true;
            }
        }
    #else
        // macOS使用GPU Family检测
        if (@available(macOS 10.15, *)) {
            // 优先使用MTLGPUFamilyMac2（macOS 10.15+）
            if (@available(macOS 13.0, *)) {
                if (![m_device supportsFamily:MTLGPUFamilyMac2]) {
                    m_signaled = false;
                    return true;
                }
            } else {
                // macOS 10.15-12.x使用Mac1
                #pragma clang diagnostic push
                #pragma clang diagnostic ignored "-Wdeprecated-declarations"
                if (![m_device supportsFamily:MTLGPUFamilyMac1]) {
                    m_signaled = false;
                    return true;
                }
                #pragma clang diagnostic pop
            }
        }
    #endif
    
    m_event = [m_device newSharedEvent];
    
    if (!m_event) {
        LR_SET_ERROR(ErrorCode::ResourceCreationFailed, "Failed to create Metal shared event");
        return false;
    }
    
    m_event.signaledValue = 0;
    m_signaledValue = 0;
    m_currentValue = 0;
    
    // 创建监听队列
    m_listenerQueue = dispatch_queue_create("com.lrengine.fence", DISPATCH_QUEUE_SERIAL);
    m_listener = [[MTLSharedEventListener alloc] initWithDispatchQueue:m_listenerQueue];
    
    return true;
}

void FenceMTL::Destroy() {
    m_event = nil;
    m_listener = nil;
    m_listenerQueue = nil;
    m_signaledValue = 0;
    m_currentValue = 0;
    m_signaled = false;
}

void FenceMTL::Signal() {
    if (m_event) {
        m_signaledValue++;
        m_event.signaledValue = m_signaledValue;
    } else {
        m_signaled = true;
    }
}

bool FenceMTL::Wait(uint64_t timeoutNs) {
    if (!m_event) {
        // 简化实现：直接检查标志
        return m_signaled;
    }
    
    // 如果已经达到目标值，直接返回
    if (m_event.signaledValue >= m_signaledValue) {
        return true;
    }
    
    // 创建等待信号量
    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    
    // 注册通知回调
    [m_event notifyListener:m_listener
                    atValue:m_signaledValue
                      block:^(id<MTLSharedEvent> event, uint64_t value) {
        (void)event;
        (void)value;
        dispatch_semaphore_signal(semaphore);
    }];
    
    // 计算超时时间
    dispatch_time_t timeout;
    if (timeoutNs == UINT64_MAX) {
        timeout = DISPATCH_TIME_FOREVER;
    } else {
        timeout = dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(timeoutNs));
    }
    
    // 等待
    long result = dispatch_semaphore_wait(semaphore, timeout);
    
    return result == 0;
}

FenceStatus FenceMTL::GetStatus() const {
    if (!m_event) {
        return m_signaled ? FenceStatus::Signaled : FenceStatus::Unsignaled;
    }
    
    if (m_event.signaledValue >= m_signaledValue) {
        return FenceStatus::Signaled;
    }
    
    return FenceStatus::Unsignaled;
}

void FenceMTL::Reset() {
    if (m_event) {
        m_signaledValue = m_event.signaledValue;
    }
    m_signaled = false;
}

ResourceHandle FenceMTL::GetNativeHandle() const {
    return ResourceHandle((__bridge void*)m_event);
}

void FenceMTL::SignalOnCommandBuffer(id<MTLCommandBuffer> commandBuffer) {
    if (!m_event || !commandBuffer) {
        return;
    }
    
    m_signaledValue++;
    [commandBuffer encodeSignalEvent:m_event value:m_signaledValue];
}

} // namespace mtl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
