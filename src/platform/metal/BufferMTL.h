/**
 * @file BufferMTL.h
 * @brief Metal缓冲区实现
 */

#pragma once

#include "platform/interface/IBufferImpl.h"

#ifdef LRENGINE_ENABLE_METAL

#import <Metal/Metal.h>

namespace lrengine {
namespace render {
namespace mtl {

/**
 * @brief Metal缓冲区实现
 */
class BufferMTL : public IBufferImpl {
public:
    BufferMTL(id<MTLDevice> device);
    ~BufferMTL() override;

    // IBufferImpl接口
    bool Create(const BufferDescriptor& desc) override;
    void Destroy() override;
    void UpdateData(const void* data, size_t size, size_t offset) override;
    void* Map(MemoryAccess access) override;
    void Unmap() override;
    void Bind() override;
    void Unbind() override;
    ResourceHandle GetNativeHandle() const override;
    size_t GetSize() const override;
    BufferUsage GetUsage() const override;
    BufferType GetType() const override;

    // Metal特有方法
    id<MTLBuffer> GetBuffer() const { return m_buffer; }

protected:
    id<MTLDevice> m_device;
    id<MTLBuffer> m_buffer;
    size_t m_size;
    BufferUsage m_bufferUsage;
    BufferType m_bufferType;
    bool m_mapped;
};

/**
 * @brief Metal顶点缓冲区实现
 */
class VertexBufferMTL : public BufferMTL {
public:
    VertexBufferMTL(id<MTLDevice> device);
    ~VertexBufferMTL() override;

    bool Create(const BufferDescriptor& desc) override;
    void SetVertexLayout(const VertexLayoutDescriptor& layout) override;

    const VertexLayoutDescriptor& GetVertexLayout() const { return m_layout; }
    uint32_t GetStride() const { return m_stride; }

private:
    VertexLayoutDescriptor m_layout;
    uint32_t m_stride;
};

/**
 * @brief Metal索引缓冲区实现
 */
class IndexBufferMTL : public BufferMTL {
public:
    IndexBufferMTL(id<MTLDevice> device);
    ~IndexBufferMTL() override;

    bool Create(const BufferDescriptor& desc) override;

    IndexType GetIndexType() const { return m_indexType; }
    uint32_t GetIndexCount() const;

private:
    IndexType m_indexType;
};

/**
 * @brief Metal Uniform缓冲区实现
 */
class UniformBufferMTL : public BufferMTL {
public:
    UniformBufferMTL(id<MTLDevice> device);
    ~UniformBufferMTL() override;

    bool Create(const BufferDescriptor& desc) override;
    void BindToSlot(uint32_t slot);

private:
    uint32_t m_bindingSlot;
};

} // namespace mtl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
