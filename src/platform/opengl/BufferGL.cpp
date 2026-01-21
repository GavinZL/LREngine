/**
 * @file BufferGL.cpp
 * @brief OpenGL缓冲区实现
 */

#include "BufferGL.h"
#include "lrengine/core/LRError.h"
#include "lrengine/utils/LRLog.h"

#ifdef LRENGINE_ENABLE_OPENGL

namespace lrengine {
namespace render {
namespace gl {

// ============================================================================
// BufferGL
// ============================================================================

BufferGL::BufferGL()
    : m_bufferID(0)
    , m_target(GL_ARRAY_BUFFER)
    , m_usage(GL_STATIC_DRAW)
    , m_size(0)
    , m_bufferUsage(BufferUsage::Static)
    , m_bufferType(BufferType::Vertex)
    , m_mapped(false) {}

BufferGL::~BufferGL() { Destroy(); }

bool BufferGL::Create(const BufferDescriptor& desc) {
    if (m_bufferID != 0) {
        Destroy();
    }

    m_bufferType  = desc.type;
    m_bufferUsage = desc.usage;
    m_size        = desc.size;
    m_target      = ToGLBufferTarget(desc.type);
    m_usage       = ToGLBufferUsage(desc.usage);

    glGenBuffers(1, &m_bufferID);
    if (m_bufferID == 0) {
        LR_SET_ERROR(ErrorCode::ResourceCreationFailed, "Failed to generate OpenGL buffer");
        return false;
    }

    glBindBuffer(m_target, m_bufferID);
    glBufferData(m_target, static_cast<GLsizeiptr>(m_size), desc.data, m_usage);
    glBindBuffer(m_target, 0);

    return true;
}

void BufferGL::Destroy() {
    if (m_bufferID != 0) {
        if (m_mapped) {
            Unmap();
        }
        glDeleteBuffers(1, &m_bufferID);
        m_bufferID = 0;
    }
    m_size = 0;
}

void BufferGL::UpdateData(const void* data, size_t size, size_t offset) {
    if (m_bufferID == 0 || data == nullptr) {
        return;
    }

    if (offset + size > m_size) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Buffer update out of bounds");
        return;
    }

    glBindBuffer(m_target, m_bufferID);
    glBufferSubData(m_target, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), data);
    glBindBuffer(m_target, 0);
}

void* BufferGL::Map(MemoryAccess access) {
    if (m_bufferID == 0 || m_mapped) {
        return nullptr;
    }

    glBindBuffer(m_target, m_bufferID);
    void* ptr = glMapBufferRange(m_target, 0, static_cast<GLsizeiptr>(m_size), ToGLMapAccess(access));

    if (ptr != nullptr) {
        m_mapped = true;
    }

    return ptr;
}

void BufferGL::Unmap() {
    if (m_bufferID == 0 || !m_mapped) {
        return;
    }

    glBindBuffer(m_target, m_bufferID);
    glUnmapBuffer(m_target);
    glBindBuffer(m_target, 0);
    m_mapped = false;
}

void BufferGL::Bind() {
    if (m_bufferID != 0) {
        glBindBuffer(m_target, m_bufferID);
        LR_LOG_DEBUG_F("OpenGL Bind Buffer: %d", m_bufferID);
    }
}

void BufferGL::Unbind() { glBindBuffer(m_target, 0); }

ResourceHandle BufferGL::GetNativeHandle() const {
    ResourceHandle handle;
    handle.glHandle = m_bufferID;
    return handle;
}

size_t BufferGL::GetSize() const { return m_size; }

BufferUsage BufferGL::GetUsage() const { return m_bufferUsage; }

BufferType BufferGL::GetType() const { return m_bufferType; }

// ============================================================================
// VertexBufferGL
// ============================================================================

VertexBufferGL::VertexBufferGL() : m_vao(0), m_stride(0) {}

VertexBufferGL::~VertexBufferGL() { Destroy(); }

bool VertexBufferGL::Create(const BufferDescriptor& desc) {
    // 创建VAO
    glGenVertexArrays(1, &m_vao);
    if (m_vao == 0) {
        LR_SET_ERROR(ErrorCode::ResourceCreationFailed, "Failed to generate VAO");
        return false;
    }

    m_stride = desc.stride;

    // 创建VBO
    return BufferGL::Create(desc);
}

void VertexBufferGL::Destroy() {
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    BufferGL::Destroy();
}

void VertexBufferGL::Bind() {
    if (m_vao != 0) {
        glBindVertexArray(m_vao);
    }
    BufferGL::Bind();
}

void VertexBufferGL::Unbind() {
    BufferGL::Unbind();
    glBindVertexArray(0);
}

void VertexBufferGL::SetVertexLayout(const VertexLayoutDescriptor& layout) {
    if (!layout.attributes.empty()) {
        m_stride = layout.stride;
        SetVertexAttributes(layout.attributes.data(),
                            static_cast<uint32_t>(layout.attributes.size()));
    }
}

void VertexBufferGL::SetVertexAttributes(const VertexAttribute* attributes, uint32_t count) {
    if (m_vao == 0 || attributes == nullptr || count == 0) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Invalid vertex attributes");
        return;
    }

    glBindVertexArray(m_vao);
    BufferGL::Bind();

    for (uint32_t i = 0; i < count; ++i) {
        const VertexAttribute& attr = attributes[i];

        glEnableVertexAttribArray(attr.location);

        GLenum type          = GetVertexFormatType(attr.format);
        GLint components     = GetVertexFormatComponents(attr.format);
        GLboolean normalized = IsVertexFormatNormalized(attr.format);

        // 整数类型使用glVertexAttribIPointer
        if (type == GL_INT || type == GL_UNSIGNED_INT || type == GL_SHORT ||
            type == GL_UNSIGNED_SHORT || type == GL_BYTE || type == GL_UNSIGNED_BYTE) {
            if (!normalized) {
                glVertexAttribIPointer(attr.location, components, type,
                                       static_cast<GLsizei>(m_stride),
                                       reinterpret_cast<const void*>(
                                           static_cast<uintptr_t>(attr.offset)));
                continue;
            }
        }

        glVertexAttribPointer(attr.location, components, type, normalized,
                              static_cast<GLsizei>(m_stride),
                              reinterpret_cast<const void*>(static_cast<uintptr_t>(attr.offset)));
    }

    glBindVertexArray(0);
    BufferGL::Unbind();
}

// ============================================================================
// IndexBufferGL
// ============================================================================

IndexBufferGL::IndexBufferGL() : m_indexType(IndexType::UInt32) {}

IndexBufferGL::~IndexBufferGL() {}

bool IndexBufferGL::Create(const BufferDescriptor& desc) {
    m_indexType = desc.indexType;
    return BufferGL::Create(desc);
}

uint32_t IndexBufferGL::GetIndexCount() const {
    size_t indexSize = (m_indexType == IndexType::UInt16) ? sizeof(uint16_t) : sizeof(uint32_t);
    return static_cast<uint32_t>(GetSize() / indexSize);
}

// ============================================================================
// UniformBufferGL
// ============================================================================

UniformBufferGL::UniformBufferGL() : m_bindingSlot(0) {}

UniformBufferGL::~UniformBufferGL() {}

bool UniformBufferGL::Create(const BufferDescriptor& desc) { return BufferGL::Create(desc); }

void UniformBufferGL::BindToSlot(uint32_t slot) {
    m_bindingSlot = slot;
    glBindBufferBase(GL_UNIFORM_BUFFER, slot, GetBufferID());
}

} // namespace gl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGL
