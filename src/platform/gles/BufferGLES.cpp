/**
 * @file BufferGLES.cpp
 * @brief OpenGL ES缓冲区实现
 */

#include "BufferGLES.h"
#include "lrengine/core/LRError.h"
#include "lrengine/utils/LRLog.h"

#ifdef LRENGINE_ENABLE_OPENGLES

namespace lrengine {
namespace render {
namespace gles {

// ============================================================================
// BufferGLES
// ============================================================================

BufferGLES::BufferGLES()
    : m_bufferID(0)
    , m_target(GL_ARRAY_BUFFER)
    , m_usage(GL_STATIC_DRAW)
    , m_size(0)
    , m_bufferUsage(BufferUsage::Static)
    , m_bufferType(BufferType::Vertex)
    , m_mapped(false)
    , m_mappedData(nullptr)
    , m_mapAccess(MemoryAccess::ReadOnly)
{
}

BufferGLES::~BufferGLES()
{
    Destroy();
}

bool BufferGLES::Create(const BufferDescriptor& desc)
{
    if (m_bufferID != 0) {
        Destroy();
    }

    m_size = desc.size;
    m_bufferUsage = desc.usage;
    m_bufferType = desc.type;
    m_target = ToGLESBufferTarget(desc.type);
    m_usage = ToGLESBufferUsage(desc.usage);

    glGenBuffers(1, &m_bufferID);
    if (m_bufferID == 0) {
        LR_SET_ERROR(ErrorCode::ResourceCreationFailed, "Failed to generate buffer");
        return false;
    }

    glBindBuffer(m_target, m_bufferID);
    glBufferData(m_target, m_size, desc.data, m_usage);
    glBindBuffer(m_target, 0);

    LR_LOG_DEBUG_F("OpenGL ES Buffer created: ID=%u, size=%zu, type=%d", 
                   m_bufferID, m_size, (int)desc.type);
    return true;
}

void BufferGLES::Destroy()
{
    if (m_mappedData) {
        delete[] m_mappedData;
        m_mappedData = nullptr;
    }
    
    if (m_bufferID != 0) {
        glDeleteBuffers(1, &m_bufferID);
        m_bufferID = 0;
    }
    m_size = 0;
    m_mapped = false;
}

void BufferGLES::UpdateData(const void* data, size_t size, size_t offset)
{
    if (m_bufferID == 0 || data == nullptr) {
        return;
    }

    if (offset + size > m_size) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Buffer update exceeds buffer size");
        return;
    }

    glBindBuffer(m_target, m_bufferID);
    glBufferSubData(m_target, offset, size, data);
    glBindBuffer(m_target, 0);
}

void* BufferGLES::Map(MemoryAccess access)
{
    if (m_bufferID == 0 || m_mapped) {
        return nullptr;
    }

    m_mapAccess = access;
    glBindBuffer(m_target, m_bufferID);

    // OpenGL ES 3.0支持glMapBufferRange
    GLbitfield flags = 0;
    switch (access) {
        case MemoryAccess::ReadOnly:
            flags = GL_MAP_READ_BIT;
            break;
        case MemoryAccess::WriteOnly:
            flags = GL_MAP_WRITE_BIT;
            break;
        case MemoryAccess::ReadWrite:
            flags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
            break;
    }

    void* ptr = glMapBufferRange(m_target, 0, m_size, flags);
    
    if (ptr == nullptr) {
        // glMapBufferRange可能失败，使用软件回退
        LR_LOG_WARNING("glMapBufferRange failed, using software fallback");
        m_mappedData = new uint8_t[m_size];
        
        if (access == MemoryAccess::ReadOnly || access == MemoryAccess::ReadWrite) {
            // 需要读取数据时，使用glGetBufferSubData（如果可用）
            // OpenGL ES没有glGetBufferSubData，只能创建新缓冲区
            LR_LOG_WARNING("Read access not fully supported in fallback mode");
        }
        
        ptr = m_mappedData;
    }

    m_mapped = true;
    glBindBuffer(m_target, 0);
    return ptr;
}

void BufferGLES::Unmap()
{
    if (!m_mapped) {
        return;
    }

    glBindBuffer(m_target, m_bufferID);

    if (m_mappedData != nullptr) {
        // 使用软件回退时，需要将数据写回
        if (m_mapAccess == MemoryAccess::WriteOnly || 
            m_mapAccess == MemoryAccess::ReadWrite) {
            glBufferSubData(m_target, 0, m_size, m_mappedData);
        }
        delete[] m_mappedData;
        m_mappedData = nullptr;
    } else {
        glUnmapBuffer(m_target);
    }

    glBindBuffer(m_target, 0);
    m_mapped = false;
}

void BufferGLES::Bind()
{
    if (m_bufferID != 0) {
        glBindBuffer(m_target, m_bufferID);
        
        // 对于EBO，检查VAO状态
        if (m_target == GL_ELEMENT_ARRAY_BUFFER) {
            GLint currentVAO = 0;
            glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
            LR_LOG_INFO_F("[BufferGLES] Bind EBO: ID=%u, CurrentVAO=%d (EBO binding will be recorded in VAO)",
                          m_bufferID, currentVAO);
            if (currentVAO == 0) {
                LR_LOG_WARNING("[BufferGLES] WARNING: EBO bound without VAO! EBO binding won't be recorded.");
            }
        } else {
            LR_LOG_TRACE_F("[BufferGLES] Bind buffer: target=0x%x, ID=%u", m_target, m_bufferID);
        }
    }
}

void BufferGLES::Unbind()
{
    glBindBuffer(m_target, 0);
}

ResourceHandle BufferGLES::GetNativeHandle() const
{
    ResourceHandle handle;
    handle.glHandle = m_bufferID;
    return handle;
}

size_t BufferGLES::GetSize() const { return m_size; }
BufferUsage BufferGLES::GetUsage() const { return m_bufferUsage; }
BufferType BufferGLES::GetType() const { return m_bufferType; }

// ============================================================================
// VertexBufferGLES
// ============================================================================

VertexBufferGLES::VertexBufferGLES()
    : m_vao(0)
    , m_stride(0)
{
    m_bufferType = BufferType::Vertex;
    m_target = GL_ARRAY_BUFFER;
}

VertexBufferGLES::~VertexBufferGLES()
{
    Destroy();
}

bool VertexBufferGLES::Create(const BufferDescriptor& desc)
{
    // 创建VAO
    glGenVertexArrays(1, &m_vao);
    if (m_vao == 0) {
        LR_SET_ERROR(ErrorCode::ResourceCreationFailed, "Failed to generate VAO");
        return false;
    }

    m_stride = desc.stride;

    // 调用基类创建VBO
    return BufferGLES::Create(desc);
}

void VertexBufferGLES::Destroy()
{
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    BufferGLES::Destroy();
}

void VertexBufferGLES::Bind()
{
    if (m_vao != 0) {
        glBindVertexArray(m_vao);
        LR_LOG_INFO_F("[BufferGLES] BindVertexArray: VAO=%u", m_vao);
        
        // 验证绑定是否成功
        GLint currentVAO = 0;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
        if (currentVAO != (GLint)m_vao) {
            LR_LOG_ERROR_F("[BufferGLES] ERROR: VAO bind failed! Expected %u, got %d", 
                          m_vao, currentVAO);
        }
    } else {
        LR_LOG_ERROR("[BufferGLES] BindVertexArray called with invalid VAO (0)!");
    }
}

void VertexBufferGLES::Unbind()
{
    glBindVertexArray(0);
}

void VertexBufferGLES::SetVertexLayout(const VertexLayoutDescriptor& layout)
{
    SetVertexAttributes(layout.attributes.data(), 
                       static_cast<uint32_t>(layout.attributes.size()));
    if (layout.stride > 0) {
        m_stride = layout.stride;
    }
}

void VertexBufferGLES::SetVertexAttributes(const VertexAttribute* attributes, uint32_t count)
{
    if (m_vao == 0 || attributes == nullptr || count == 0) {
        LR_LOG_ERROR_F("[BufferGLES] SetVertexAttributes: invalid params (VAO=%u, attrs=%p, count=%u)",
                       m_vao, attributes, count);
        return;
    }

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_bufferID);
    
    LR_LOG_INFO_F("[BufferGLES] SetVertexAttributes: VAO=%u, VBO=%u, count=%u, stride=%u",
                  m_vao, m_bufferID, count, m_stride);

    for (uint32_t i = 0; i < count; ++i) {
        const VertexAttribute& attr = attributes[i];
        
        GLint components = GetGLESVertexFormatComponents(attr.format);
        GLenum type = GetGLESVertexFormatType(attr.format);
        GLboolean normalized = IsGLESVertexFormatNormalized(attr.format);
        GLsizei stride = (attr.stride > 0) ? attr.stride : m_stride;

        glEnableVertexAttribArray(attr.location);
        
        LR_LOG_INFO_F("[BufferGLES]   Attr[%u]: location=%u, components=%d, type=0x%x, stride=%d, offset=%u",
                      i, attr.location, components, type, stride, attr.offset);
        
        // 整数类型使用glVertexAttribIPointer
        if (type == GL_INT || type == GL_UNSIGNED_INT || 
            type == GL_SHORT || type == GL_UNSIGNED_SHORT ||
            type == GL_BYTE || type == GL_UNSIGNED_BYTE) {
            if (!normalized) {
                glVertexAttribIPointer(attr.location, components, type, stride,
                                       reinterpret_cast<const void*>(attr.offset));
                continue;
            }
        }
        
        glVertexAttribPointer(attr.location, components, type, normalized, stride,
                             reinterpret_cast<const void*>(attr.offset));
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    LR_LOG_INFO_F("[BufferGLES] SetVertexAttributes completed: VAO=%u, count=%u", m_vao, count);
}

// ============================================================================
// IndexBufferGLES
// ============================================================================

IndexBufferGLES::IndexBufferGLES()
    : m_indexType(IndexType::UInt32)
{
    m_bufferType = BufferType::Index;
    m_target = GL_ELEMENT_ARRAY_BUFFER;
}

IndexBufferGLES::~IndexBufferGLES()
{
    Destroy();
}

bool IndexBufferGLES::Create(const BufferDescriptor& desc)
{
    m_indexType = desc.indexType;
    return BufferGLES::Create(desc);
}

uint32_t IndexBufferGLES::GetIndexCount() const
{
    size_t indexSize = (m_indexType == IndexType::UInt16) ? 2 : 4;
    return static_cast<uint32_t>(m_size / indexSize);
}

// ============================================================================
// UniformBufferGLES
// ============================================================================

UniformBufferGLES::UniformBufferGLES()
    : m_bindingSlot(0)
{
    m_bufferType = BufferType::Uniform;
    m_target = GL_UNIFORM_BUFFER;
}

UniformBufferGLES::~UniformBufferGLES()
{
    Destroy();
}

bool UniformBufferGLES::Create(const BufferDescriptor& desc)
{
    return BufferGLES::Create(desc);
}

void UniformBufferGLES::BindToSlot(uint32_t slot)
{
    if (m_bufferID != 0) {
        m_bindingSlot = slot;
        glBindBufferBase(GL_UNIFORM_BUFFER, slot, m_bufferID);
        LR_LOG_TRACE_F("OpenGL ES BindUniformBuffer: slot=%u, ID=%u", slot, m_bufferID);
    }
}

} // namespace gles
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_OPENGLES
