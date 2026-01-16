# LREngine - 跨平台渲染引擎

## 概述

LREngine 是一个轻量级、高性能的跨平台渲染引擎，采用现代 C++17 编写。它提供了一套统一的渲染抽象层，支持多种图形后端（当前实现 OpenGL 3.3+），使开发者能够编写一次代码，在多个平台上运行。

### 主要特性

- **跨平台支持**: macOS、Windows、Linux
- **多后端架构**: 可扩展的后端设计，当前支持 OpenGL 3.3+
- **现代 C++ API**: 使用 C++17 特性，提供类型安全的接口
- **资源管理**: 引用计数和 RAII 模式的资源生命周期管理
- **状态缓存**: OpenGL 状态缓存优化，减少冗余 API 调用
- **错误处理**: 完善的错误码系统和回调机制

---

## 目录结构

```
LREngine/
├── include/lrengine/          # 公共头文件
│   ├── core/                  # 核心模块
│   │   ├── LRDefines.h        # 宏定义和平台检测
│   │   ├── LRTypes.h          # 类型定义和描述符
│   │   ├── LRError.h          # 错误处理
│   │   ├── LRResource.h       # 资源基类
│   │   ├── LRBuffer.h         # 缓冲区对象
│   │   ├── LRShader.h         # 着色器对象
│   │   ├── LRTexture.h        # 纹理对象
│   │   ├── LRFrameBuffer.h    # 帧缓冲对象
│   │   ├── LRPipelineState.h  # 管线状态
│   │   ├── LRFence.h          # 同步屏障
│   │   └── LRRenderContext.h  # 渲染上下文
│   ├── factory/               # 工厂模块
│   │   └── LRDeviceFactory.h  # 设备工厂
│   └── math/                  # 数学库
│       └── lrmath.h           # GLM 数学库封装
├── src/                       # 源文件
│   ├── core/                  # 核心实现
│   └── platform/              # 平台特定实现
│       ├── interface/         # 平台接口定义
│       └── opengl/            # OpenGL 后端实现
├── examples/                  # 示例程序
│   └── HelloTriangle/         # 三角形渲染示例
├── Doc/                       # 文档
└── CMakeLists.txt             # 构建配置
```

---

## 快速开始

### 环境要求

- CMake 3.16+
- C++17 兼容编译器
- OpenGL 3.3+ 支持
- GLFW 3.3+ (示例程序依赖)

### 构建步骤

```bash
# 克隆仓库
git clone <repository-url>
cd LREngine

# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译
make -j4

# 运行示例
./bin/examples/HelloTriangle
```

### 最小示例

```cpp
#include <GLFW/glfw3.h>
#include "lrengine/core/LRRenderContext.h"
#include "lrengine/core/LRBuffer.h"
#include "lrengine/core/LRShader.h"

using namespace lrengine::render;

int main() {
    // 1. 初始化 GLFW 并创建窗口
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow* window = glfwCreateWindow(800, 600, "LREngine", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // 2. 创建渲染上下文
    RenderContextDescriptor contextDesc;
    contextDesc.backend = Backend::OpenGL;
    contextDesc.width = 800;
    contextDesc.height = 600;
    contextDesc.windowHandle = window;
    
    LRRenderContext* context = LRRenderContext::Create(contextDesc);

    // 3. 创建着色器
    const char* vsSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        void main() {
            gl_Position = vec4(aPos, 1.0);
        }
    )";
    
    const char* fsSource = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(1.0, 0.5, 0.2, 1.0);
        }
    )";

    ShaderDescriptor vsDesc;
    vsDesc.stage = ShaderStage::Vertex;
    vsDesc.source = vsSource;
    LRShader* vertexShader = context->CreateShader(vsDesc);

    ShaderDescriptor fsDesc;
    fsDesc.stage = ShaderStage::Fragment;
    fsDesc.source = fsSource;
    LRShader* fragmentShader = context->CreateShader(fsDesc);

    LRShaderProgram* program = context->CreateShaderProgram(vertexShader, fragmentShader);

    // 4. 创建顶点缓冲
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };

    BufferDescriptor vbDesc;
    vbDesc.size = sizeof(vertices);
    vbDesc.type = BufferType::Vertex;
    vbDesc.usage = BufferUsage::Static;
    vbDesc.data = vertices;
    vbDesc.stride = 3 * sizeof(float);
    
    LRVertexBuffer* vertexBuffer = context->CreateVertexBuffer(vbDesc);

    // 5. 设置顶点布局
    VertexLayoutDescriptor layout;
    layout.stride = 3 * sizeof(float);
    
    VertexAttribute posAttr;
    posAttr.location = 0;
    posAttr.format = VertexFormat::Float3;
    posAttr.offset = 0;
    layout.attributes.push_back(posAttr);
    
    vertexBuffer->SetVertexLayout(layout);

    // 6. 渲染循环
    while (!glfwWindowShouldClose(window)) {
        context->Clear(ClearFlag_Color | ClearFlag_Depth, 0.2f, 0.3f, 0.3f, 1.0f);
        
        program->Use();
        context->SetVertexBuffer(vertexBuffer, 0);
        context->Draw(0, 3);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 7. 清理
    delete vertexBuffer;
    delete program;
    delete fragmentShader;
    delete vertexShader;
    LRRenderContext::Destroy(context);
    
    glfwTerminate();
    return 0;
}
```

---

## 核心模块

### LRRenderContext - 渲染上下文

渲染上下文是引擎的核心入口点，负责资源创建和渲染命令提交。

```cpp
// 创建上下文
LRRenderContext* context = LRRenderContext::Create(desc);

// 资源创建
LRVertexBuffer* vb = context->CreateVertexBuffer(bufferDesc);
LRIndexBuffer* ib = context->CreateIndexBuffer(bufferDesc);
LRShader* shader = context->CreateShader(shaderDesc);
LRShaderProgram* program = context->CreateShaderProgram(vs, fs);
LRTexture* texture = context->CreateTexture(textureDesc);
LRFrameBuffer* fb = context->CreateFrameBuffer(fbDesc);
LRPipelineState* ps = context->CreatePipelineState(psDesc);

// 渲染状态
context->SetViewport(0, 0, width, height);
context->SetVertexBuffer(vb, 0);
context->SetIndexBuffer(ib);
context->SetTexture(texture, 0);
context->SetPipelineState(ps);

// 清除和绘制
context->Clear(ClearFlag_Color | ClearFlag_Depth, r, g, b, a, depth, stencil);
context->Draw(vertexStart, vertexCount);
context->DrawIndexed(indexStart, indexCount);

// 销毁
LRRenderContext::Destroy(context);
```

### LRBuffer - 缓冲区

支持三种缓冲区类型：

| 类型 | 类 | 用途 |
|------|-----|------|
| Vertex | `LRVertexBuffer` | 顶点数据存储 |
| Index | `LRIndexBuffer` | 索引数据存储 |
| Uniform | `LRUniformBuffer` | Uniform 数据存储 |

```cpp
// 顶点缓冲
BufferDescriptor vbDesc;
vbDesc.size = dataSize;
vbDesc.type = BufferType::Vertex;
vbDesc.usage = BufferUsage::Static;  // Static/Dynamic/Stream
vbDesc.data = vertexData;
vbDesc.stride = sizeof(Vertex);

LRVertexBuffer* vb = context->CreateVertexBuffer(vbDesc);

// 设置顶点布局
VertexLayoutDescriptor layout;
layout.stride = sizeof(Vertex);
layout.attributes = { ... };
vb->SetVertexLayout(layout);

// 更新数据
vb->UpdateData(newData, size, offset);

// 映射/解映射
void* ptr = vb->Map(MemoryAccess::WriteOnly);
// ... 写入数据 ...
vb->Unmap();
```

### LRShader - 着色器

```cpp
// 创建着色器
ShaderDescriptor desc;
desc.stage = ShaderStage::Vertex;  // Vertex/Fragment/Geometry/Compute
desc.source = shaderSource;
desc.debugName = "MyShader";

LRShader* shader = context->CreateShader(desc);

if (!shader->IsCompiled()) {
    std::cerr << shader->GetCompileError() << std::endl;
}

// 创建着色器程序
LRShaderProgram* program = context->CreateShaderProgram(vs, fs, gs);

if (!program->IsLinked()) {
    std::cerr << program->GetLinkError() << std::endl;
}

// 使用程序
program->Use();

// 设置 Uniform
int32_t loc = program->GetUniformLocation("uMVP");
program->SetUniformMatrix4fv(loc, mvpMatrix, false);
```

### LRTexture - 纹理

```cpp
TextureDescriptor desc;
desc.width = 512;
desc.height = 512;
desc.type = TextureType::Texture2D;  // 2D/3D/Cube/2DArray/2DMultisample
desc.format = PixelFormat::RGBA8;
desc.mipLevels = 1;  // 0 = 自动计算
desc.sampleCount = 1;
desc.data = pixelData;
desc.generateMipmaps = true;

// 采样器配置
desc.sampler.minFilter = FilterMode::Linear;
desc.sampler.magFilter = FilterMode::Linear;
desc.sampler.wrapU = WrapMode::Repeat;
desc.sampler.wrapV = WrapMode::Repeat;
desc.sampler.maxAnisotropy = 16.0f;

LRTexture* texture = context->CreateTexture(desc);

// 绑定到纹理单元
texture->Bind(0);

// 更新纹理数据
texture->UpdateData(newData, mipLevel, &region);

// 生成 Mipmap
texture->GenerateMipmaps();
```

### LRFrameBuffer - 帧缓冲

```cpp
FrameBufferDescriptor desc;
desc.width = 1024;
desc.height = 1024;

LRFrameBuffer* fb = context->CreateFrameBuffer(desc);

// 附加颜色纹理
fb->AttachColorTexture(colorTexture, 0, 0);  // texture, slot, mipLevel

// 附加深度纹理
fb->AttachDepthTexture(depthTexture, 0);

// 检查完整性
if (!fb->IsComplete()) {
    // 处理错误
}

// 开始渲染到帧缓冲
context->BeginRenderPass(fb);
// ... 渲染命令 ...
context->EndRenderPass();
```

### LRPipelineState - 管线状态

```cpp
PipelineStateDescriptor desc;

// 混合状态
desc.blendState.enabled = true;
desc.blendState.srcFactor = BlendFactor::SrcAlpha;
desc.blendState.dstFactor = BlendFactor::OneMinusSrcAlpha;
desc.blendState.equation = BlendEquation::Add;

// 深度模板状态
desc.depthStencilState.depthTestEnabled = true;
desc.depthStencilState.depthWriteEnabled = true;
desc.depthStencilState.depthCompareFunc = CompareFunc::Less;
desc.depthStencilState.stencilTestEnabled = false;

// 光栅化状态
desc.rasterizerState.cullMode = CullMode::Back;
desc.rasterizerState.frontFace = FrontFace::CounterClockwise;
desc.rasterizerState.fillMode = FillMode::Solid;

// 图元类型
desc.primitiveType = PrimitiveType::Triangles;

LRPipelineState* ps = context->CreatePipelineState(desc);
context->SetPipelineState(ps);
```

---

## 错误处理

LREngine 提供统一的错误处理机制：

```cpp
#include "lrengine/core/LRError.h"

// 设置错误回调
LRError::SetErrorCallback([](const ErrorInfo& error) {
    std::cerr << "[" << static_cast<int>(error.code) << "] "
              << error.message
              << " at " << error.file << ":" << error.line
              << std::endl;
});

// 检查错误
if (LRError::HasError()) {
    ErrorCode code = LRError::GetLastError();
    const ErrorInfo& info = LRError::GetLastErrorInfo();
    // 处理错误...
    LRError::ClearError();
}
```

### 错误码分类

| 范围 | 类别 | 示例 |
|------|------|------|
| 1-99 | 通用错误 | InvalidArgument, OutOfMemory |
| 100-199 | 设备错误 | DeviceCreationFailed, DeviceLost |
| 200-299 | 资源错误 | ResourceCreationFailed, BufferMapFailed |
| 300-399 | 着色器错误 | ShaderCompileFailed, ShaderLinkFailed |
| 400-499 | 纹理错误 | TextureCreationFailed, TextureFormatNotSupported |
| 500-599 | 帧缓冲错误 | FrameBufferIncomplete |
| 600-699 | 管线错误 | PipelineCreationFailed |
| 700-799 | 同步错误 | FenceTimeout, FenceError |

---

## 类型定义

### 顶点格式

```cpp
enum class VertexFormat {
    Float, Float2, Float3, Float4,      // 浮点
    Int, Int2, Int3, Int4,              // 有符号整数
    UInt, UInt2, UInt3, UInt4,          // 无符号整数
    Short2, Short4,                      // 16位整数
    UShort2, UShort4,                    // 无符号16位
    Byte4, UByte4,                       // 8位整数
    Byte4Normalized, UByte4Normalized,   // 归一化8位
    // ...
};
```

### 像素格式

```cpp
enum class PixelFormat {
    // 8位格式
    R8, RG8, RGB8, RGBA8,
    R8_SNORM, RG8_SNORM, RGB8_SNORM, RGBA8_SNORM,
    
    // 16位格式
    R16F, RG16F, RGB16F, RGBA16F,
    R16, RG16, RGB16, RGBA16,
    
    // 32位格式
    R32F, RG32F, RGB32F, RGBA32F,
    R32I, RG32I, RGB32I, RGBA32I,
    R32UI, RG32UI, RGB32UI, RGBA32UI,
    
    // 深度/模板格式
    Depth16, Depth24, Depth32F,
    Depth24Stencil8, Depth32FStencil8,
    Stencil8,
    
    // 压缩格式
    DXT1, DXT3, DXT5,
    // ...
};
```

---

## 架构设计

### 层次结构

```
┌─────────────────────────────────────────┐
│           Application Layer             │
│         (用户应用程序代码)                │
├─────────────────────────────────────────┤
│           Public API Layer              │
│  LRRenderContext, LRBuffer, LRShader... │
├─────────────────────────────────────────┤
│         Interface Layer                 │
│  IRenderContextImpl, IBufferImpl...     │
├─────────────────────────────────────────┤
│         Backend Layer                   │
│  OpenGL: ContextGL, BufferGL...         │
│  (Future: Vulkan, Metal, D3D12...)      │
└─────────────────────────────────────────┘
```

### 设计模式

- **抽象工厂**: `LRDeviceFactory` 用于创建特定后端的实现
- **策略模式**: 管线状态配置
- **RAII**: 资源生命周期管理
- **引用计数**: `LRResource` 基类提供引用计数支持

---

## 性能优化

### OpenGL 状态缓存

`StateCacheGL` 类缓存 OpenGL 状态以减少冗余的 API 调用：

```cpp
// 内部实现示例
void StateCacheGL::UseProgram(GLuint program) {
    if (m_currentProgram != program) {
        glUseProgram(program);
        m_currentProgram = program;
    }
}
```

缓存的状态包括：
- 当前着色器程序
- VAO 绑定
- 缓冲区绑定
- 纹理绑定
- 帧缓冲绑定
- 视口和裁剪矩形
- 混合状态
- 深度测试状态
- 背面剔除状态

---

## 扩展指南

### 添加新的图形后端

1. 在 `src/platform/` 下创建新目录（如 `vulkan/`）
2. 实现所有接口类：
   - `IRenderContextImpl`
   - `IBufferImpl`
   - `IShaderImpl`
   - `IShaderProgramImpl`
   - `ITextureImpl`
   - `IFrameBufferImpl`
   - `IPipelineStateImpl`
   - `IFenceImpl`
3. 创建设备工厂类继承 `LRDeviceFactory`
4. 在 `LRDeviceFactory::GetFactory()` 中注册新后端

---

## 已知限制

- 当前仅实现 OpenGL 3.3+ 后端
- macOS 上 OpenGL 已被废弃（仍可使用但会有警告）
- 不支持计算着色器（需要 OpenGL 4.3+）
- 不支持曲面细分着色器（需要 OpenGL 4.0+）

---

## 许可证

[待定]

---

## 联系方式

[待定]
