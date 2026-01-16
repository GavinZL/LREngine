/**
 * @file BufferGL.h
 * @brief OpenGL缓冲区实现
 */

#pragma once

#include "platform/interface/IBufferImpl.h"
#include "TypeConverterGL.h"

#ifdef LRENGINE_ENABLE_OPENGL

namespace lrengine {
namespace render {
namespace gl {

/**
 * @brief OpenGL缓冲区实现
 */
class BufferGL : public IBufferImpl {
public:
    BufferGL();
    ~BufferGL() override;

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

    // OpenGL特有方法
    GLuint GetBufferID() const { return m_bufferID; }
    GLenum GetTarget() const { return m_target; }

private:
    GLuint m_bufferID;
    GLenum m_target;
    GLenum m_usage;
    size_t m_size;
    BufferUsage m_bufferUsage;
    BufferType m_bufferType;
    bool m_mapped;
};

/**
 * @brief OpenGL顶点缓冲区实现（包含VAO管理）
 */
class VertexBufferGL : public BufferGL {
public:
    VertexBufferGL();
    ~VertexBufferGL() override;

    bool Create(const BufferDescriptor& desc) override;
    void Destroy() override;
    void Bind() override;
    void Unbind() override;
    void SetVertexLayout(const VertexLayoutDescriptor& layout) override;

    // 顶点属性配置
    void SetVertexAttributes(const VertexAttribute* attributes, uint32_t count);
    GLuint GetVAO() const { return m_vao; }

private:
    GLuint m_vao;
    uint32_t m_stride;
};

/**
 * @brief OpenGL索引缓冲区实现
 */
class IndexBufferGL : public BufferGL {
public:
    IndexBufferGL();
    ~IndexBufferGL() override;

    bool Create(const BufferDescriptor& desc) override;

    IndexType GetIndexType() const { return m_indexType; }
    GLenum GetGLIndexType() const { return ToGLIndexType(m_indexType); }
    uint32_t GetIndexCount() const;

private:
    IndexType m_indexType;
};

/**
 * @brief OpenGL Uniform缓冲区实现
 */
class UniformBufferGL : public BufferGL {
public:
    UniformBufferGL();
    ~UniformBufferGL() override;

    bool Create(const BufferDescriptor& desc) override;
    void BindToSlot(uint32_t slot);

private:
    uint32_t m_bindingSlot;
};

} // namespace gl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGL
