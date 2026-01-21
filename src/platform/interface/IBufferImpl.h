/**
 * @file IBufferImpl.h
 * @brief 缓冲区平台实现接口
 */

#pragma once

#include "lrengine/core/LRTypes.h"

namespace lrengine {
namespace render {

/**
 * @brief 缓冲区实现接口
 * 
 * 所有平台后端必须实现此接口
 */
class IBufferImpl {
public:
    virtual ~IBufferImpl() = default;

    /**
     * @brief 创建缓冲区
     * @param desc 缓冲区描述符
     * @return 成功返回true
     */
    virtual bool Create(const BufferDescriptor& desc) = 0;

    /**
     * @brief 销毁缓冲区
     */
    virtual void Destroy() = 0;

    /**
     * @brief 更新缓冲区数据
     * @param data 数据指针
     * @param size 数据大小
     * @param offset 缓冲区偏移
     */
    virtual void UpdateData(const void* data, size_t size, size_t offset) = 0;

    /**
     * @brief 映射缓冲区到CPU地址空间
     * @param access 访问模式
     * @return 映射的指针，失败返回nullptr
     */
    virtual void* Map(MemoryAccess access) = 0;

    /**
     * @brief 取消映射
     */
    virtual void Unmap() = 0;

    /**
     * @brief 绑定缓冲区
     */
    virtual void Bind() = 0;

    /**
     * @brief 解绑缓冲区
     */
    virtual void Unbind() = 0;

    /**
     * @brief 获取原生句柄
     */
    virtual ResourceHandle GetNativeHandle() const = 0;

    /**
     * @brief 获取缓冲区大小
     */
    virtual size_t GetSize() const = 0;

    /**
     * @brief 获取使用模式
     */
    virtual BufferUsage GetUsage() const = 0;

    /**
     * @brief 获取缓冲区类型
     */
    virtual BufferType GetType() const = 0;

    /**
     * @brief 设置顶点布局（仅顶点缓冲区需要实现）
     * @param layout 顶点布局描述符
     */
    virtual void SetVertexLayout(const VertexLayoutDescriptor& layout) {
        LR_UNUSED(layout);
        // 默认空实现，非顶点缓冲区不需要
    }
};

} // namespace render
} // namespace lrengine
