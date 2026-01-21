/**
 * @file IFenceImpl.h
 * @brief 同步栅栏平台实现接口
 */

#pragma once

#include "lrengine/core/LRTypes.h"

namespace lrengine {
namespace render {

/**
 * @brief 同步栅栏实现接口
 */
class IFenceImpl {
public:
    virtual ~IFenceImpl() = default;

    /**
     * @brief 创建栅栏
     * @return 成功返回true
     */
    virtual bool Create() = 0;

    /**
     * @brief 销毁栅栏
     */
    virtual void Destroy() = 0;

    /**
     * @brief 发送信号（GPU端）
     */
    virtual void Signal() = 0;

    /**
     * @brief 等待信号（CPU端）
     * @param timeoutNs 超时时间（纳秒），UINT64_MAX表示无限等待
     * @return 成功返回true，超时返回false
     */
    virtual bool Wait(uint64_t timeoutNs) = 0;

    /**
     * @brief 获取状态（非阻塞）
     */
    virtual FenceStatus GetStatus() const = 0;

    /**
     * @brief 重置栅栏
     */
    virtual void Reset() = 0;

    /**
     * @brief 获取原生句柄
     */
    virtual ResourceHandle GetNativeHandle() const = 0;
};

} // namespace render
} // namespace lrengine
