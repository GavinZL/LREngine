/**
 * @file CommandBufferPoolMTL.mm
 * @brief Metal命令缓冲池实现
 */

#include "CommandBufferPoolMTL.h"
#include "lrengine/utils/LRLog.h"
#include <thread>
#include <chrono>

#ifdef LRENGINE_ENABLE_METAL

namespace lrengine {
namespace render {
namespace mtl {

CommandBufferPoolMTL::CommandBufferPoolMTL(id<MTLCommandQueue> queue, uint32_t maxBuffers)
    : m_queue(queue)
    , m_maxBuffers(maxBuffers)
{
    LR_LOG_INFO_F("CommandBufferPoolMTL: Created pool with max %u buffers", maxBuffers);
}

CommandBufferPoolMTL::~CommandBufferPoolMTL() {
    WaitIdle();
    m_inFlightBuffers.clear();
}

id<MTLCommandBuffer> CommandBufferPoolMTL::AcquireCommandBuffer() {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    // 如果达到上限，等待直到有buffer完成
    if (m_inFlightBuffers.size() >= m_maxBuffers) {
        LR_LOG_WARNING_F("CommandBufferPoolMTL: Reached max buffer limit (%u), waiting...", m_maxBuffers);
        
        m_condition.wait(lock, [this] {
            return m_inFlightBuffers.size() < m_maxBuffers;
        });
    }
    
    // 创建新的CommandBuffer（Metal的CommandBuffer是一次性的，不能复用）
    id<MTLCommandBuffer> cmdBuffer = [m_queue commandBuffer];
    m_inFlightBuffers.push_back(cmdBuffer);
    
    LR_LOG_INFO_F("CommandBufferPoolMTL: Created new buffer (total in-flight: %zu)",
                m_inFlightBuffers.size());
    
    return cmdBuffer;
}

void CommandBufferPoolMTL::SubmitCommandBuffer(id<MTLCommandBuffer> cmdBuffer) {
    if (!cmdBuffer) {
        return;
    }
    
    // 注册完成回调，buffer完成后从in-flight队列移除
    __weak CommandBufferPoolMTL* weakSelf = this;
    [cmdBuffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull buffer) {
        CommandBufferPoolMTL* strongSelf = weakSelf;
        if (!strongSelf) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(strongSelf->m_mutex);
        
        // 从in-flight队列移除（CommandBuffer一次性使用，完成后销毁，不回收）
        auto it = std::find(strongSelf->m_inFlightBuffers.begin(),
                           strongSelf->m_inFlightBuffers.end(),
                           buffer);
        if (it != strongSelf->m_inFlightBuffers.end()) {
            strongSelf->m_inFlightBuffers.erase(it);
        }
        
        LR_LOG_INFO_F("CommandBufferPoolMTL: Buffer completed (in-flight: %zu)",
                    strongSelf->m_inFlightBuffers.size());
        
        // 通知等待线程，现在可以创建新buffer了
        strongSelf->m_condition.notify_one();
    }];
    
    // 提交
    [cmdBuffer commit];
}

void CommandBufferPoolMTL::WaitIdle() {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    if (m_inFlightBuffers.empty()) {
        return;
    }
    
    LR_LOG_INFO_F("CommandBufferPoolMTL: Waiting for %zu in-flight buffers...",
                m_inFlightBuffers.size());
    
    // 使用超时等待，避免程序退出时永久阻塞
    // 在退出场景下，包含presentDrawable的CommandBuffer可能永远不会完成
    const int maxWaitSeconds = 2;  // 最多等待2秒
    const int checkIntervalMs = 100;  // 每100ms检查一次
    int totalWaitMs = 0;
    
    while (!m_inFlightBuffers.empty() && totalWaitMs < maxWaitSeconds * 1000) {
        // 临时解锁，让completion handler有机会执行
        lock.unlock();
        
        // 检查所有buffer的状态
        bool allCompleted = true;
        std::vector<id<MTLCommandBuffer>> buffersToCheck;
        {
            std::lock_guard<std::mutex> checkLock(m_mutex);
            buffersToCheck = m_inFlightBuffers;
        }
        
        for (id<MTLCommandBuffer> cmdBuffer : buffersToCheck) {
            MTLCommandBufferStatus status = [cmdBuffer status];
            if (status != MTLCommandBufferStatusCompleted &&
                status != MTLCommandBufferStatusError) {
                allCompleted = false;
                break;
            }
        }
        
        if (allCompleted) {
            break;
        }
        
        // 等待一小段时间
        std::this_thread::sleep_for(std::chrono::milliseconds(checkIntervalMs));
        totalWaitMs += checkIntervalMs;
        
        lock.lock();
    }
    
    if (!m_inFlightBuffers.empty()) {
        LR_LOG_WARNING_F("CommandBufferPoolMTL: Timeout waiting for %zu buffers, forcing cleanup",
                       m_inFlightBuffers.size());
        
        // 强制清空队列，允许程序继续
        // 这些buffer可能仍在GPU中，但我们即将销毁整个上下文
        m_inFlightBuffers.clear();
    }
    
    LR_LOG_INFO_F("CommandBufferPoolMTL: WaitIdle completed");
}

uint32_t CommandBufferPoolMTL::GetInFlightCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<uint32_t>(m_inFlightBuffers.size());
}

} // namespace mtl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
