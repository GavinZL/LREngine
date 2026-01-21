/**
 * @file BufferGLES.h
 * @brief OpenGL ES缓冲区实现
 */

#pragma once

#include "platform/interface/IBufferImpl.h"
#include "TypeConverterGLES.h"

#ifdef LRENGINE_ENABLE_OPENGLES

namespace lrengine {
namespace render {
namespace gles {

/**
 * @brief OpenGL ES缓冲区基类实现
 */
class BufferGLES : public IBufferImpl {
public:
    BufferGLES();
    ~BufferGLES() override;

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

    // OpenGL ES特有方法
    GLuint GetBufferID() const { return m_bufferID; }
    GLenum GetTarget() const { return m_target; }

protected:
    GLuint m_bufferID;
    GLenum m_target;
    GLenum m_usage;
    size_t m_size;
    BufferUsage m_bufferUsage;
    BufferType m_bufferType;
    bool m_mapped;

    // Map回退方案（当glMapBufferRange不可用时）
    uint8_t* m_mappedData;
    MemoryAccess m_mapAccess;
};

/**
 * @brief OpenGL ES顶点缓冲区实现（包含VAO管理）
 */
class VertexBufferGLES : public BufferGLES {
public:
    VertexBufferGLES();
    ~VertexBufferGLES() override;

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
 * @brief OpenGL ES索引缓冲区实现
 */
class IndexBufferGLES : public BufferGLES {
public:
    IndexBufferGLES();
    ~IndexBufferGLES() override;

    bool Create(const BufferDescriptor& desc) override;

    IndexType GetIndexType() const { return m_indexType; }
    GLenum GetGLIndexType() const { return ToGLESIndexType(m_indexType); }
    uint32_t GetIndexCount() const;

private:
    IndexType m_indexType;
};

/**
 * @brief OpenGL ES Uniform缓冲区实现
 */
class UniformBufferGLES : public BufferGLES {
public:
    UniformBufferGLES();
    ~UniformBufferGLES() override;

    bool Create(const BufferDescriptor& desc) override;
    void BindToSlot(uint32_t slot);

private:
    uint32_t m_bindingSlot;
};

} // namespace gles
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGLES
