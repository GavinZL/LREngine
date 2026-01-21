/**
 * @file CommandBufferPoolMTL.h
 * @brief Metal命令缓冲池
 * 
 * 支持多线程并行记录命令，通过池化管理减少创建开销
 */

#pragma once

#include <vector>
#include <mutex>
#include <condition_variable>

#ifdef LRENGINE_ENABLE_METAL

#import <Metal/Metal.h>

namespace lrengine {
namespace render {
namespace mtl {

/**
 * @brief Metal命令缓冲池
 * 
 * 功能：
 * 1. 控制CommandBuffer的并发数量（流控）
 * 2. CommandBuffer是一次性使用的，不会被复用
 * 3. 支持多线程安全的并发访问
 * 4. 当达到上限时阻塞等待，直到有buffer完成
 */
class CommandBufferPoolMTL {
public:
    explicit CommandBufferPoolMTL(id<MTLCommandQueue> queue, uint32_t maxBuffers = 3);
    ~CommandBufferPoolMTL();

    /**
     * @brief 获取一个CommandBuffer
     * 
     * 线程安全，如果达到并发上限会阻塞等待
     * 注意：返回的是新创建的CommandBuffer，不是复用的
     */
    id<MTLCommandBuffer> AcquireCommandBuffer();

    /**
     * @brief 提交CommandBuffer
     * 
     * 会自动注册completion handler，完成后从in-flight队列移除
     */
    void SubmitCommandBuffer(id<MTLCommandBuffer> cmdBuffer);

    /**
     * @brief 等待所有CommandBuffer完成
     */
    void WaitIdle();

    /**
     * @brief 获取正在使用的CommandBuffer数量
     */
    uint32_t GetInFlightCount() const;

private:
    id<MTLCommandQueue> m_queue;
    uint32_t m_maxBuffers;

    // 线程同步
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;

    // 正在使用的缓冲区（CommandBuffer是一次性的，完成后销毁，不回收）
    std::vector<id<MTLCommandBuffer>> m_inFlightBuffers;
};

} // namespace mtl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
