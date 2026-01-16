/**
 * @file LRBuffer.h
 * @brief LREngine缓冲区对象
 */

#pragma once

#include "LRResource.h"
#include "LRTypes.h"

namespace lrengine {
namespace render {

class IBufferImpl;

/**
 * @brief 通用缓冲区类
 * 
 * GPU缓冲区的抽象封装，支持顶点、索引、Uniform数据存储
 */
class LR_API LRBuffer : public LRResource {
public:
    LR_NONCOPYABLE(LRBuffer);
    
    virtual ~LRBuffer();
    
    /**
     * @brief 更新缓冲区数据
     * @param data 数据指针
     * @param size 数据大小（字节）
     * @param offset 缓冲区偏移（字节）
     */
    void UpdateData(const void* data, size_t size, size_t offset = 0);
    
    /**
     * @brief 映射缓冲区到CPU地址空间
     * @param access 访问模式
     * @return 映射的指针，失败返回nullptr
     */
    void* Map(MemoryAccess access = MemoryAccess::WriteOnly);
    
    /**
     * @brief 取消映射
     */
    void Unmap();
    
    /**
     * @brief 绑定缓冲区
     */
    void Bind();
    
    /**
     * @brief 解绑缓冲区
     */
    void Unbind();
    
    /**
     * @brief 获取缓冲区大小
     */
    size_t GetSize() const { return mSize; }
    
    /**
     * @brief 获取使用模式
     */
    BufferUsage GetUsage() const { return mUsage; }
    
    /**
     * @brief 获取缓冲区类型
     */
    BufferType GetBufferType() const { return mBufferType; }
    
    /**
     * @brief 获取原生句柄
     */
    ResourceHandle GetNativeHandle() const override;
    
    /**
     * @brief 获取平台实现
     */
    IBufferImpl* GetImpl() const { return mImpl; }
    
protected:
    friend class LRRenderContext;
    
    /**
     * @brief 构造函数（通过LRRenderContext创建）
     */
    LRBuffer();
    
    /**
     * @brief 初始化缓冲区
     * @param impl 平台实现
     * @param desc 描述符
     */
    bool Initialize(IBufferImpl* impl, const BufferDescriptor& desc);
    
protected:
    IBufferImpl* mImpl = nullptr;
    size_t mSize = 0;
    BufferUsage mUsage = BufferUsage::Static;
    BufferType mBufferType = BufferType::Vertex;
};

/**
 * @brief 顶点缓冲区类
 */
class LR_API LRVertexBuffer : public LRBuffer {
public:
    LR_NONCOPYABLE(LRVertexBuffer);
    
    virtual ~LRVertexBuffer() = default;
    
    /**
     * @brief 设置顶点布局
     * @param layout 顶点布局描述符
     */
    void SetVertexLayout(const VertexLayoutDescriptor& layout);
    
    /**
     * @brief 获取顶点布局
     */
    const VertexLayoutDescriptor& GetVertexLayout() const { return mVertexLayout; }
    
    /**
     * @brief 获取顶点数量
     */
    uint32_t GetVertexCount() const;
    
protected:
    friend class LRRenderContext;
    
    LRVertexBuffer();
    bool Initialize(IBufferImpl* impl, const BufferDescriptor& desc);
    
protected:
    VertexLayoutDescriptor mVertexLayout;
};

/**
 * @brief 索引缓冲区类
 */
class LR_API LRIndexBuffer : public LRBuffer {
public:
    LR_NONCOPYABLE(LRIndexBuffer);
    
    virtual ~LRIndexBuffer() = default;
    
    /**
     * @brief 设置索引类型
     */
    void SetIndexType(IndexType type) { mIndexType = type; }
    
    /**
     * @brief 获取索引类型
     */
    IndexType GetIndexType() const { return mIndexType; }
    
    /**
     * @brief 获取索引数量
     */
    uint32_t GetIndexCount() const;
    
protected:
    friend class LRRenderContext;
    
    LRIndexBuffer();
    bool Initialize(IBufferImpl* impl, const BufferDescriptor& desc);
    
protected:
    IndexType mIndexType = IndexType::UInt32;
};

/**
 * @brief 统一缓冲区类
 */
class LR_API LRUniformBuffer : public LRBuffer {
public:
    LR_NONCOPYABLE(LRUniformBuffer);
    
    virtual ~LRUniformBuffer() = default;
    
    /**
     * @brief 设置绑定点
     */
    void SetBindingPoint(uint32_t point) { mBindingPoint = point; }
    
    /**
     * @brief 获取绑定点
     */
    uint32_t GetBindingPoint() const { return mBindingPoint; }
    
    /**
     * @brief 绑定到指定绑定点
     * @param point 绑定点索引
     */
    void BindToPoint(uint32_t point);
    
protected:
    friend class LRRenderContext;
    
    LRUniformBuffer();
    bool Initialize(IBufferImpl* impl, const BufferDescriptor& desc);
    
protected:
    uint32_t mBindingPoint = 0;
};

} // namespace render
} // namespace lrengine
