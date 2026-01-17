/**
 * @file BufferMTL.mm
 * @brief Metal缓冲区实现
 */

#include "BufferMTL.h"

#ifdef LRENGINE_ENABLE_METAL

#include "TypeConverterMTL.h"
#include "lrengine/core/LRError.h"

namespace lrengine {
namespace render {
namespace mtl {

// =============================================================================
// BufferMTL
// =============================================================================

BufferMTL::BufferMTL(id<MTLDevice> device)
    : m_device(device)
    , m_buffer(nil)
    , m_size(0)
    , m_bufferUsage(BufferUsage::Static)
    , m_bufferType(BufferType::Vertex)
    , m_mapped(false)
{
}

BufferMTL::~BufferMTL() {
    Destroy();
}

bool BufferMTL::Create(const BufferDescriptor& desc) {
    if (desc.size == 0) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Buffer size cannot be zero");
        return false;
    }

    m_size = desc.size;
    m_bufferUsage = desc.usage;
    m_bufferType = desc.type;

    MTLResourceOptions options = ToMTLResourceOptions(desc.usage);

    if (desc.data) {
        m_buffer = [m_device newBufferWithBytes:desc.data
                                         length:desc.size
                                        options:options];
    } else {
        m_buffer = [m_device newBufferWithLength:desc.size
                                         options:options];
    }

    if (!m_buffer) {
        LR_SET_ERROR(ErrorCode::ResourceCreationFailed, "Failed to create Metal buffer");
        return false;
    }

    if (desc.debugName) {
        m_buffer.label = [NSString stringWithUTF8String:desc.debugName];
    }

    return true;
}

void BufferMTL::Destroy() {
    m_buffer = nil;
    m_size = 0;
    m_mapped = false;
}

void BufferMTL::UpdateData(const void* data, size_t size, size_t offset) {
    if (!m_buffer || !data) {
        LR_SET_ERROR(ErrorCode::InvalidState, "Buffer or data is null");
        return;
    }

    if (offset + size > m_size) {
        LR_SET_ERROR(ErrorCode::InvalidArgument, "Buffer update exceeds buffer size");
        return;
    }

    void* contents = [m_buffer contents];
    if (contents) {
        memcpy(static_cast<uint8_t*>(contents) + offset, data, size);
    }
}

void* BufferMTL::Map(MemoryAccess access) {
    LR_UNUSED(access);
    
    if (!m_buffer) {
        LR_SET_ERROR(ErrorCode::InvalidState, "Buffer is null");
        return nullptr;
    }

    m_mapped = true;
    return [m_buffer contents];
}

void BufferMTL::Unmap() {
    m_mapped = false;
    // Metal使用共享存储模式时不需要显式同步
}

void BufferMTL::Bind() {
    // Metal中缓冲区绑定在编码命令时完成
    // 对于Uniform缓冲区，需要通知RenderContext进行绑定
    if (m_bufferType == BufferType::Uniform) {
        // 通过RenderContext的ContextMTL接口进行实际绑定
        // 这将在SetUniformBuffer时调用
    }
}

void BufferMTL::Unbind() {
    // Metal中缓冲区绑定在编码命令时完成
}

ResourceHandle BufferMTL::GetNativeHandle() const {
    return ResourceHandle((__bridge void*)m_buffer);
}

size_t BufferMTL::GetSize() const {
    return m_size;
}

BufferUsage BufferMTL::GetUsage() const {
    return m_bufferUsage;
}

BufferType BufferMTL::GetType() const {
    return m_bufferType;
}

// =============================================================================
// VertexBufferMTL
// =============================================================================

VertexBufferMTL::VertexBufferMTL(id<MTLDevice> device)
    : BufferMTL(device)
    , m_stride(0)
{
    m_bufferType = BufferType::Vertex;
}

VertexBufferMTL::~VertexBufferMTL() = default;

bool VertexBufferMTL::Create(const BufferDescriptor& desc) {
    if (!BufferMTL::Create(desc)) {
        LR_SET_ERROR(ErrorCode::ResourceCreationFailed, "Failed to create vertex buffer");
        return false;
    }
    
    m_stride = desc.stride;
    return true;
}

void VertexBufferMTL::SetVertexLayout(const VertexLayoutDescriptor& layout) {
    m_layout = layout;
    if (layout.stride > 0) {
        m_stride = layout.stride;
    }
}

// =============================================================================
// IndexBufferMTL
// =============================================================================

IndexBufferMTL::IndexBufferMTL(id<MTLDevice> device)
    : BufferMTL(device)
    , m_indexType(IndexType::UInt32)
{
    m_bufferType = BufferType::Index;
}

IndexBufferMTL::~IndexBufferMTL() = default;

bool IndexBufferMTL::Create(const BufferDescriptor& desc) {
    if (!BufferMTL::Create(desc)) {
        return false;
    }
    
    m_indexType = desc.indexType;
    return true;
}

uint32_t IndexBufferMTL::GetIndexCount() const {
    size_t indexSize = GetIndexTypeSize(m_indexType);
    return static_cast<uint32_t>(m_size / indexSize);
}

// =============================================================================
// UniformBufferMTL
// =============================================================================

UniformBufferMTL::UniformBufferMTL(id<MTLDevice> device)
    : BufferMTL(device)
    , m_bindingSlot(0)
{
    m_bufferType = BufferType::Uniform;
}

UniformBufferMTL::~UniformBufferMTL() = default;

bool UniformBufferMTL::Create(const BufferDescriptor& desc) {
    return BufferMTL::Create(desc);
}

void UniformBufferMTL::BindToSlot(uint32_t slot) {
    m_bindingSlot = slot;
}

} // namespace mtl
} // namespace render
} // namespace lrengine

#endif // LRENGINE_ENABLE_METAL
