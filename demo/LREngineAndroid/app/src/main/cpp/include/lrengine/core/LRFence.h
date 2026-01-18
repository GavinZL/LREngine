/**
 * @file LRFence.h
 * @brief LREngine同步栅栏对象
 */

#pragma once

#include "LRResource.h"
#include "LRTypes.h"

namespace lrengine {
namespace render {

class IFenceImpl;

/**
 * @brief 同步栅栏类
 * 
 * 用于GPU-CPU同步：
 * - GPU完成特定操作后发送信号
 * - CPU等待信号以确保GPU操作完成
 */
class LR_API LRFence : public LRResource {
public:
    LR_NONCOPYABLE(LRFence);
    
    virtual ~LRFence();
    
    /**
     * @brief 发送信号（GPU端）
     * 
     * 插入一个同步点，当GPU执行到此点时触发信号
     */
    void Signal();
    
    /**
     * @brief 等待信号（CPU端）
     * @param timeoutNs 超时时间（纳秒），UINT64_MAX表示无限等待
     * @return 成功返回true，超时返回false
     */
    bool Wait(uint64_t timeoutNs = UINT64_MAX);
    
    /**
     * @brief 获取状态（非阻塞）
     */
    FenceStatus GetStatus() const;
    
    /**
     * @brief 重置栅栏到未触发状态
     */
    void Reset();
    
    /**
     * @brief 检查是否已触发
     */
    bool IsSignaled() const { return GetStatus() == FenceStatus::Signaled; }
    
    /**
     * @brief 获取原生句柄
     */
    ResourceHandle GetNativeHandle() const override;
    
protected:
    friend class LRRenderContext;
    
    LRFence();
    bool Initialize(IFenceImpl* impl);
    
protected:
    IFenceImpl* mImpl = nullptr;
};

} // namespace render
} // namespace lrengine
