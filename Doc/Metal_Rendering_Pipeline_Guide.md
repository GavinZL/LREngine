# LREngine Metal渲染流程详解

> **版本**: 1.0  
> **最后更新**: 2026-01-18  
> **作者**: LREngine团队

---

## 目录

1. [Metal渲染架构概述](#1-metal渲染架构概述)
2. [渲染通道管理](#2-渲染通道管理)
3. [CommandBuffer池化管理](#3-commandbuffer池化管理)
4. [多渲染目标(MRT)支持](#4-多渲染目标mrt支持)
5. [渲染状态管理](#5-渲染状态管理)
6. [资源生命周期](#6-资源生命周期)
7. [与OpenGL后端的差异对比](#7-与opengl后端的差异对比)
8. [常见问题和解决方案](#8-常见问题和解决方案)

---

## 1. Metal渲染架构概述

### 1.1 架构设计目标

LREngine的Metal后端经过完整重构，旨在：
- ✅ **显式管理渲染通道**：遵循Metal的显式设计哲学
- ✅ **支持多渲染目标(MRT)**：满足延迟渲染等高级技术需求
- ✅ **线程安全**：通过CommandBuffer池化支持多线程渲染
- ✅ **嵌套渲染支持**：状态栈机制支持复杂渲染流程（阴影→主渲染→后处理）
- ✅ **资源高效管理**：避免资源泄漏和同步问题

### 1.2 核心组件

```
┌─────────────────────────────────────────────────────────────┐
│                    RenderContextMTL                         │
│  Metal渲染上下文 - 整体协调者                                 │
└──────────────┬──────────────────────────────────────────────┘
               │
               ├─► CommandBufferPoolMTL
               │   命令缓冲池 - 管理GPU命令提交的并发
               │   • 流控：限制同时在飞的CommandBuffer数量
               │   • 自动回收：completion handler机制
               │
               ├─► RenderStateStackMTL
               │   渲染状态栈 - 支持嵌套RenderPass
               │   • 保存/恢复视口、裁剪等状态
               │   • 管理多层渲染流程
               │
               ├─► RenderPassMTL
               │   渲染通道抽象 - 封装RenderPassDescriptor
               │   • 支持多渲染目标(MRT)
               │   • LoadAction/StoreAction配置
               │
               └─► Metal原生对象
                   • MTLDevice
                   • MTLCommandQueue
                   • CAMetalLayer
```

### 1.3 渲染流程总览

```cpp
// 标准渲染循环
while (rendering) {
    // 1. 开始帧 - 获取drawable和CommandBuffer
    context->BeginFrame();
    
    // 2. 开始渲染通道 - 创建RenderCommandEncoder
    context->BeginRenderPass(nullptr);  // nullptr = 渲染到屏幕
    
    // 3. 设置管线和资源
    shaderProgram->Use();
    context->SetPipelineState(pipelineState);
    context->SetVertexBuffer(vertexBuffer, 0);
    
    // 4. 提交绘制命令
    context->Draw(0, 3);
    
    // 5. 结束渲染通道 - 关闭encoder
    context->EndRenderPass();
    
    // 6. 结束帧 - presentDrawable并提交CommandBuffer
    context->EndFrame();
}
```

---

## 2. 渲染通道管理

### 2.1 核心概念

**Metal的RenderCommandEncoder必须通过RenderPass创建，不能像OpenGL那样"随时"绘制。**

```
帧开始 → 创建RenderPass → 创建Encoder → 绘制命令 → 结束Encoder → 提交帧
         ↑                   ↑              ↑             ↑
         BeginRenderPass     自动           Draw()        EndRenderPass
```

### 2.2 API设计

#### 新旧API对比

```cpp
// ❌ 旧API（兼容OpenGL，不推荐）
context->BeginRenderPass(IFrameBufferImpl* fb);

// ✅ 新API（Metal原生，推荐）
context->BeginRenderPassEx(RenderPassMTL* renderPass);
context->BeginMRTRenderPass(colorTargets, depthTarget);
context->BeginOffscreenRenderPass(target, depthTarget);
```

#### 实现机制

```cpp
void RenderContextMTL::BeginRenderPass(IFrameBufferImpl* frameBuffer) {
    // 接口桥接层
    if (frameBuffer) {
        // 离屏渲染
        FrameBufferMTL* fb = static_cast<FrameBufferMTL*>(frameBuffer);
        BeginOffscreenRenderPass(fb, nullptr);
    } else {
        // 默认framebuffer（屏幕）
        BeginRenderPassEx(nullptr);
    }
}

void RenderContextMTL::BeginRenderPassEx(RenderPassMTL* renderPass) {
    if (!m_frameStarted) {
        LR_LOG_ERROR("BeginRenderPass called but frame not started");
        return;
    }
    
    // 1. 获取或创建RenderPass
    if (!renderPass) {
        // 使用默认RenderPass（渲染到屏幕）
        RenderPassConfig config;
        config.useDefaultFrameBuffer = true;
        config.drawable = m_currentDrawable;
        config.defaultDepthTexture = m_depthStencilTexture;
        
        m_currentRenderPass = std::make_unique<RenderPassMTL>(config);
        renderPass = m_currentRenderPass.get();
    }
    
    // 2. 创建RenderCommandEncoder
    id<MTLRenderCommandEncoder> encoder = CreateRenderEncoder(renderPass);
    if (!encoder) {
        LR_LOG_ERROR("Failed to create render encoder");
        return;
    }
    
    // 3. 保存到状态栈
    RenderState state;
    state.renderPass = renderPass;
    state.encoder = encoder;
    state.commandBuffer = m_frameCommandBuffer;
    state.viewport = m_viewport;
    state.scissor = m_scissorRect;
    state.depth = m_stateStack->GetDepth();
    
    m_stateStack->PushState(state);
    
    // 4. 应用视口和裁剪
    ApplyViewportAndScissor();
}
```

### 2.3 状态栈管理

**RenderStateStackMTL支持嵌套渲染通道，实现多Pass渲染**：

```cpp
// 示例：阴影贴图 + 主渲染
context->BeginFrame();

// Pass 1: 渲染阴影贴图
context->BeginRenderPass(shadowMapFB);  // 压栈，深度=1
RenderShadowCasters();
context->EndRenderPass();               // 出栈，深度=0

// Pass 2: 主渲染
context->BeginRenderPass(nullptr);      // 压栈，深度=1
RenderMainScene();
context->EndRenderPass();               // 出栈，深度=0

context->EndFrame();
```

**状态栈结构**：

```cpp
struct RenderState {
    RenderPassMTL* renderPass;              // 当前RenderPass
    id<MTLRenderCommandEncoder> encoder;    // 关联的encoder
    id<MTLCommandBuffer> commandBuffer;     // 关联的CommandBuffer
    MTLViewport viewport;                   // 视口状态
    MTLScissorRect scissor;                 // 裁剪状态
    PipelineStateMTL* pipelineState;        // 管线状态
    uint32_t depth;                         // 栈深度（调试用）
};
```

---

## 3. CommandBuffer池化管理

### 3.1 设计原理

**Metal的CommandBuffer是一次性使用的**，不能像OpenGL的上下文那样复用。CommandBufferPoolMTL的作用是：

1. **流控**：限制同时在GPU执行的CommandBuffer数量（默认3个）
2. **避免过度提交**：CPU提交速度快于GPU执行时，自动阻塞等待
3. **自动回收**：通过completion handler机制，buffer完成后自动从池中移除

### 3.2 生命周期管理

```
AcquireCommandBuffer → 记录命令 → SubmitCommandBuffer → GPU执行 → CompletionHandler
       ↓                                                                    ↓
  加入in-flight队列                                                  从in-flight移除
       ↓                                                                    ↓
  计数+1                                                              计数-1
```

#### 代码实现

```cpp
id<MTLCommandBuffer> CommandBufferPoolMTL::AcquireCommandBuffer() {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    // 如果达到上限，等待直到有buffer完成
    if (m_inFlightBuffers.size() >= m_maxBuffers) {
        LR_LOG_WARNING("Reached max buffer limit (%u), waiting...", m_maxBuffers);
        
        m_condition.wait(lock, [this] {
            return m_inFlightBuffers.size() < m_maxBuffers;
        });
    }
    
    // 创建新的CommandBuffer（不复用！）
    id<MTLCommandBuffer> cmdBuffer = [m_queue commandBuffer];
    m_inFlightBuffers.push_back(cmdBuffer);
    
    return cmdBuffer;
}

void CommandBufferPoolMTL::SubmitCommandBuffer(id<MTLCommandBuffer> cmdBuffer) {
    // 注册完成回调
    __weak CommandBufferPoolMTL* weakSelf = this;
    [cmdBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
        CommandBufferPoolMTL* strongSelf = weakSelf;
        if (!strongSelf) return;
        
        std::lock_guard<std::mutex> lock(strongSelf->m_mutex);
        
        // 从in-flight队列移除
        auto it = std::find(strongSelf->m_inFlightBuffers.begin(),
                           strongSelf->m_inFlightBuffers.end(),
                           buffer);
        if (it != strongSelf->m_inFlightBuffers.end()) {
            strongSelf->m_inFlightBuffers.erase(it);
        }
        
        // 通知等待线程
        strongSelf->m_condition.notify_one();
    }];
    
    // 提交到GPU
    [cmdBuffer commit];
}
```

### 3.3 帧级CommandBuffer模式

**关键设计**：一帧使用一个CommandBuffer，所有RenderPass共享。

```cpp
void RenderContextMTL::BeginFrame() {
    m_frameStarted = true;
    
    // 获取drawable
    m_currentDrawable = [m_metalLayer nextDrawable];
    
    // 为整个帧创建一个CommandBuffer
    m_frameCommandBuffer = m_commandBufferPool->AcquireCommandBuffer();
}

void RenderContextMTL::EndFrame() {
    // Present drawable
    if (m_frameCommandBuffer && m_currentDrawable) {
        [m_frameCommandBuffer presentDrawable:m_currentDrawable];
    }
    
    // 提交CommandBuffer（注册completion handler）
    if (m_frameCommandBuffer && m_commandBufferPool) {
        m_commandBufferPool->SubmitCommandBuffer(m_frameCommandBuffer);
        m_frameCommandBuffer = nil;
    }
    
    m_currentDrawable = nil;
    m_frameStarted = false;
}
```

**为什么这样设计？**

- ✅ **避免资源泄漏**：之前每个RenderPass创建新buffer，只提交最后一个，导致池耗尽
- ✅ **符合Metal最佳实践**：一帧一个CommandBuffer，多个encoder顺序记录
- ✅ **简化生命周期**：清晰的acquire/submit配对

---

## 4. 多渲染目标(MRT)支持

### 4.1 什么是MRT？

**Multi Render Target（多渲染目标）**允许一个绘制调用同时输出到多个颜色纹理。

**应用场景**：
- 延迟渲染（Deferred Rendering）：G-Buffer（Position + Normal + Albedo + ...）
- 体积光照（Volumetric Lighting）
- 屏幕空间反射（Screen Space Reflections）

### 4.2 RenderPassMTL设计

```cpp
struct RenderPassConfig {
    // 支持多个颜色附件（最多8个）
    std::vector<FrameBufferMTL*> colorTargets;
    
    // 深度附件（可选）
    FrameBufferMTL* depthTarget = nullptr;
    
    // LoadAction/StoreAction配置
    MTLLoadAction colorLoadAction = MTLLoadActionClear;
    MTLStoreAction colorStoreAction = MTLStoreActionStore;
    
    // 清除值
    float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float clearDepth = 1.0f;
};
```

### 4.3 MRT使用示例

```cpp
// 创建多个颜色纹理（G-Buffer）
LRTexture* positionTexture = CreateTexture(width, height, RGBA16F);
LRTexture* normalTexture = CreateTexture(width, height, RGBA16F);
LRTexture* albedoTexture = CreateTexture(width, height, RGBA8);

// 创建FrameBuffer
LRFrameBuffer* positionFB = CreateFrameBuffer(positionTexture);
LRFrameBuffer* normalFB = CreateFrameBuffer(normalTexture);
LRFrameBuffer* albedoFB = CreateFrameBuffer(albedoTexture);

// 开始MRT渲染
std::vector<FrameBufferMTL*> colorTargets = {
    static_cast<FrameBufferMTL*>(positionFB->GetImpl()),
    static_cast<FrameBufferMTL*>(normalFB->GetImpl()),
    static_cast<FrameBufferMTL*>(albedoFB->GetImpl())
};

context->BeginMRTRenderPass(colorTargets, depthFB);

// 片段着色器输出到多个颜色附件
/*
fragment GBufferOutput fragmentMain(VertexOut in [[stage_in]]) {
    GBufferOutput output;
    output.position = float4(in.worldPosition, 1.0);    // attachment 0
    output.normal = float4(normalize(in.normal), 1.0);  // attachment 1
    output.albedo = float4(in.color, 1.0);              // attachment 2
    return output;
}
*/

RenderScene();
context->EndRenderPass();
```

### 4.4 RenderPassDescriptor创建逻辑

```cpp
MTLRenderPassDescriptor* RenderPassMTL::CreateDescriptor() const {
    MTLRenderPassDescriptor* descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    
    // 配置多个颜色附件
    for (size_t i = 0; i < m_config.colorTargets.size(); ++i) {
        FrameBufferMTL* target = m_config.colorTargets[i];
        
        descriptor.colorAttachments[i].texture = target->GetColorTexture();
        descriptor.colorAttachments[i].loadAction = m_config.colorLoadAction;
        descriptor.colorAttachments[i].storeAction = m_config.colorStoreAction;
        descriptor.colorAttachments[i].clearColor = MTLClearColorMake(
            m_config.clearColor[0],
            m_config.clearColor[1],
            m_config.clearColor[2],
            m_config.clearColor[3]
        );
    }
    
    // 配置深度附件
    if (m_config.depthTarget) {
        descriptor.depthAttachment.texture = m_config.depthTarget->GetDepthTexture();
        descriptor.depthAttachment.loadAction = m_config.depthLoadAction;
        descriptor.depthAttachment.storeAction = m_config.depthStoreAction;
        descriptor.depthAttachment.clearDepth = m_config.clearDepth;
    }
    
    return descriptor;
}
```

---

## 5. 渲染状态管理

### 5.1 为什么需要状态栈？

**问题场景**：

```cpp
// 主渲染循环
SetViewport(0, 0, screenWidth, screenHeight);
BeginRenderPass(nullptr);

    // 嵌套：渲染阴影贴图
    SetViewport(0, 0, shadowMapSize, shadowMapSize);  // 修改视口
    BeginRenderPass(shadowMapFB);
        RenderShadowCasters();
    EndRenderPass();
    
    // 问题：此时视口仍然是shadowMapSize，而非screenSize！
    RenderMainScene();  // ❌ 视口错误！

EndRenderPass();
```

**解决方案**：状态栈自动保存/恢复状态。

### 5.2 状态栈工作流程

```
主渲染开始
│
├─ PushState { viewport: screen, encoder: mainEncoder }  [深度=1]
│  │
│  ├─ 嵌套：阴影贴图
│  ├─ PushState { viewport: shadow, encoder: shadowEncoder }  [深度=2]
│  │  RenderShadowCasters()
│  ├─ PopState → 恢复viewport=screen, encoder=mainEncoder  [深度=1]
│  │
│  RenderMainScene()  // ✅ 视口已自动恢复！
│  │
├─ PopState  [深度=0]
```

### 5.3 实现细节

```cpp
void RenderStateStackMTL::PushState(const RenderState& state) {
    m_stateStack.push_back(state);
    LR_LOG_INFO("Pushed state (depth: %u)", state.depth);
}

RenderState RenderStateStackMTL::PopState() {
    if (m_stateStack.empty()) {
        LR_LOG_ERROR("PopState called on empty stack");
        return RenderState{};
    }
    
    RenderState state = m_stateStack.back();
    m_stateStack.pop_back();
    
    LR_LOG_INFO("Popped state (depth was: %u, now: %u)", 
               state.depth, GetDepth());
    
    return state;
}

void RenderContextMTL::EndRenderPassEx() {
    if (m_stateStack->IsEmpty()) {
        LR_LOG_WARNING("EndRenderPass called but no active render pass");
        return;
    }
    
    // 1. 从栈中弹出状态
    RenderState state = m_stateStack->PopState();
    
    // 2. 结束encoder
    if (state.encoder) {
        [state.encoder endEncoding];
    }
    
    // 3. 如果栈非空，恢复父级状态
    if (!m_stateStack->IsEmpty()) {
        RenderState& parentState = m_stateStack->GetCurrentState();
        m_viewport = parentState.viewport;
        m_scissorRect = parentState.scissor;
        m_currentPipelineState = parentState.pipelineState;
        
        // 应用恢复的状态
        ApplyViewportAndScissor();
    }
}
```

---

## 6. 资源生命周期

### 6.1 Metal对象生命周期

```
创建           使用               销毁
─────────────────────────────────────────
MTLDevice      获取系统默认设备      ARC自动管理
MTLCommandQueue 创建一次，全局使用    ARC自动管理
MTLCommandBuffer 每帧创建，commit后回收  ARC+CompletionHandler
MTLRenderCommandEncoder 每个RenderPass创建  endEncoding后释放
MTLTexture     按需创建             ARC自动管理
MTLBuffer      按需创建             ARC自动管理
```

### 6.2 ContextMTL生命周期

```cpp
// 初始化
RenderContextMTL::Initialize() {
    // 1. 创建核心对象
    m_device = MTLCreateSystemDefaultDevice();
    m_commandQueue = [m_device newCommandQueue];
    m_metalLayer = [CAMetalLayer layer];
    
    // 2. 创建管理器
    m_commandBufferPool = std::make_unique<CommandBufferPoolMTL>(m_commandQueue, 3);
    m_stateStack = std::make_unique<RenderStateStackMTL>();
    
    // 3. 创建深度纹理
    CreateDepthStencilTexture();
}

// 销毁
RenderContextMTL::Shutdown() {
    // 1. 清空状态栈
    if (m_stateStack) {
        m_stateStack->Clear();
    }
    
    // 2. 等待所有CommandBuffer完成
    if (m_commandBufferPool) {
        m_commandBufferPool->WaitIdle();  // ⚠️ 关键：确保GPU完成
    }
    
    // 3. 清理管理器
    m_currentRenderPass.reset();
    m_defaultRenderPass.reset();
    m_stateStack.reset();
    m_commandBufferPool.reset();
    
    // 4. 释放Metal对象（ARC自动管理）
    m_depthStencilTexture = nil;
    m_currentDrawable = nil;
    m_metalLayer = nil;
    m_commandQueue = nil;
    m_device = nil;
}
```

### 6.3 WaitIdle超时机制

**问题**：程序退出时，包含`presentDrawable`的CommandBuffer可能永远不完成，导致`waitUntilCompleted`永久阻塞。

**解决方案**：超时+轮询检查。

```cpp
void CommandBufferPoolMTL::WaitIdle() {
    const int maxWaitSeconds = 2;      // 最多等待2秒
    const int checkIntervalMs = 100;   // 每100ms检查一次
    int totalWaitMs = 0;
    
    while (!m_inFlightBuffers.empty() && totalWaitMs < maxWaitSeconds * 1000) {
        // 检查所有buffer的状态（非阻塞）
        bool allCompleted = true;
        for (id<MTLCommandBuffer> cmdBuffer : m_inFlightBuffers) {
            MTLCommandBufferStatus status = [cmdBuffer status];
            if (status != MTLCommandBufferStatusCompleted &&
                status != MTLCommandBufferStatusError) {
                allCompleted = false;
                break;
            }
        }
        
        if (allCompleted) break;
        
        // 等待100ms
        std::this_thread::sleep_for(std::chrono::milliseconds(checkIntervalMs));
        totalWaitMs += checkIntervalMs;
    }
    
    if (!m_inFlightBuffers.empty()) {
        LR_LOG_WARNING("Timeout waiting for %zu buffers, forcing cleanup",
                      m_inFlightBuffers.size());
        m_inFlightBuffers.clear();  // 强制清空，允许程序退出
    }
}
```

---

## 7. 与OpenGL后端的差异对比

### 7.1 核心差异

| 方面 | OpenGL | Metal | 影响 |
|------|--------|-------|------|
| **渲染上下文** | 全局状态，始终active | 必须显式创建RenderCommandEncoder | Metal要求显式管理RenderPass |
| **绘制命令** | 可随时调用`glDraw*` | 必须在encoder内调用 | **必须先BeginRenderPass** |
| **命令提交** | 立即模式或延迟 | CommandBuffer批量提交 | 需要帧级管理 |
| **渲染目标** | 隐式绑定FBO | 通过RenderPassDescriptor显式指定 | RenderPass封装 |
| **多线程** | 复杂，需要多个context | 原生支持，多个CommandBuffer并行 | CommandBufferPool |
| **资源同步** | 手动Fence/Barrier | Completion handler自动 | 更简洁 |

### 7.2 API映射

```cpp
// OpenGL风格（兼容层）
context->BeginFrame();
context->Clear(ClearFlag_Color, ...);
shaderProgram->Use();
context->SetPipelineState(pipelineState);
context->Draw(0, 3);
context->EndFrame();

// Metal原生风格（推荐）
context->BeginFrame();
context->BeginRenderPass(nullptr);  // ← 必须显式创建RenderPass
shaderProgram->Use();
context->SetPipelineState(pipelineState);
context->Draw(0, 3);
context->EndRenderPass();           // ← 必须显式结束
context->EndFrame();
```

### 7.3 着色器语言差异

```glsl
// OpenGL (GLSL)
#version 330 core
in vec3 position;
out vec3 fragColor;

void main() {
    gl_Position = vec4(position, 1.0);
}
```

```metal
// Metal (MSL)
#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float3 position [[attribute(0)]];
};

vertex float4 vertexMain(VertexIn in [[stage_in]]) {
    return float4(in.position, 1.0);
}
```

---

## 8. 常见问题和解决方案

### 8.1 问题1：DrawArrays报错"no active render pass"

**症状**：
```
[ERROR] ContextMTL: DrawArrays called but no active render pass
```

**原因**：没有调用`BeginRenderPass`就尝试绘制。

**解决方案**：
```cpp
// ❌ 错误用法
context->BeginFrame();
context->Draw(0, 3);  // 错误：没有active encoder
context->EndFrame();

// ✅ 正确用法
context->BeginFrame();
context->BeginRenderPass(nullptr);  // 创建encoder
context->Draw(0, 3);
context->EndRenderPass();
context->EndFrame();
```

---

### 8.2 问题2：CommandBuffer池耗尽

**症状**：
```
[WARN] CommandBufferPoolMTL: Reached max buffer limit (3), waiting...
```

**原因1**：CPU提交速度快于GPU执行，正常的流控。

**解决方案**：无需处理，这是正常的流控机制。

**原因2**：每个RenderPass创建新CommandBuffer但只提交最后一个（资源泄漏）。

**解决方案**：使用帧级CommandBuffer模式（已修复）。

```cpp
// ❌ 旧实现（资源泄漏）
void BeginRenderPass() {
    id<MTLCommandBuffer> cmdBuffer = GetNewCommandBuffer();  // 每次新建
    // 保存到栈...
}

void EndFrame() {
    [lastCommandBuffer commit];  // 只提交最后一个
    // 前面的buffer永远不会被提交，池耗尽！
}

// ✅ 新实现（帧级CommandBuffer）
void BeginFrame() {
    m_frameCommandBuffer = m_commandBufferPool->AcquireCommandBuffer();
}

void BeginRenderPass() {
    // 所有RenderPass共享m_frameCommandBuffer
}

void EndFrame() {
    m_commandBufferPool->SubmitCommandBuffer(m_frameCommandBuffer);
    m_frameCommandBuffer = nil;
}
```

---

### 8.3 问题3：ESC退出时程序卡死

**症状**：按ESC键后程序无响应，日志停在：
```
[INFO] CommandBufferPoolMTL: Waiting for 1 in-flight buffers...
```

**原因**：最后一帧的CommandBuffer包含`presentDrawable`，退出时drawable失效，`waitUntilCompleted`永久阻塞。

**解决方案**：添加超时机制。

```cpp
void CommandBufferPoolMTL::WaitIdle() {
    // 使用超时+轮询检查状态，而非waitUntilCompleted
    const int maxWaitSeconds = 2;
    
    while (!m_inFlightBuffers.empty() && totalWaitMs < maxWaitSeconds * 1000) {
        // 检查status（非阻塞）
        MTLCommandBufferStatus status = [cmdBuffer status];
        if (status == MTLCommandBufferStatusCompleted) break;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // 超时后强制清空
    if (!m_inFlightBuffers.empty()) {
        LR_LOG_WARNING("Timeout, forcing cleanup");
        m_inFlightBuffers.clear();
    }
}
```

---

### 8.4 问题4：嵌套渲染后视口错误

**症状**：渲染阴影贴图后，主场景的视口变成阴影贴图大小。

**原因**：嵌套RenderPass修改了视口，但没有恢复。

**解决方案**：使用状态栈自动保存/恢复。

```cpp
// RenderStateStackMTL自动处理
void RenderContextMTL::BeginRenderPass() {
    RenderState state;
    state.viewport = m_viewport;  // 保存当前视口
    m_stateStack->PushState(state);
    
    // 修改视口...
}

void RenderContextMTL::EndRenderPass() {
    RenderState state = m_stateStack->PopState();
    m_viewport = state.viewport;  // 自动恢复
    ApplyViewportAndScissor();
}
```

---

### 8.5 问题5：MRT渲染黑屏

**症状**：使用MRT渲染后，所有颜色纹理都是黑色。

**可能原因**：

1. **片段着色器输出结构错误**：
```metal
// ❌ 错误
fragment float4 fragmentMain(...) {
    return float4(color, 1.0);  // 只输出到attachment 0
}

// ✅ 正确
struct GBufferOutput {
    float4 position [[color(0)]];
    float4 normal [[color(1)]];
    float4 albedo [[color(2)]];
};

fragment GBufferOutput fragmentMain(...) {
    GBufferOutput output;
    output.position = ...;
    output.normal = ...;
    output.albedo = ...;
    return output;
}
```

2. **RenderPassDescriptor配置错误**：
```cpp
// 确保所有颜色附件都正确配置
for (size_t i = 0; i < colorTargets.size(); ++i) {
    descriptor.colorAttachments[i].texture = colorTargets[i]->GetColorTexture();
    descriptor.colorAttachments[i].storeAction = MTLStoreActionStore;  // 必须Store
}
```

---

## 9. 最佳实践

### 9.1 渲染流程模板

```cpp
// 标准单Pass渲染
void RenderFrame() {
    context->BeginFrame();
    
    context->Clear(ClearFlag_Color | ClearFlag_Depth, ...);
    context->BeginRenderPass(nullptr);
    
    shaderProgram->Use();
    context->SetPipelineState(pipelineState);
    context->SetVertexBuffer(vertexBuffer, 0);
    context->Draw(0, vertexCount);
    
    context->EndRenderPass();
    context->EndFrame();
}

// 多Pass渲染（阴影+主场景）
void RenderFrameWithShadows() {
    context->BeginFrame();
    
    // Pass 1: 阴影贴图
    context->BeginRenderPass(shadowMapFB);
    RenderShadowCasters();
    context->EndRenderPass();
    
    // Pass 2: 主场景
    context->BeginRenderPass(nullptr);
    RenderMainScene();
    context->EndRenderPass();
    
    context->EndFrame();
}

// 离屏渲染+后处理
void RenderWithPostProcessing() {
    context->BeginFrame();
    
    // Pass 1: 渲染到离屏纹理
    context->BeginRenderPass(offscreenFB);
    RenderScene();
    context->EndRenderPass();
    
    // Pass 2: 后处理到屏幕
    context->BeginRenderPass(nullptr);
    ApplyPostProcessing(offscreenTexture);
    context->EndRenderPass();
    
    context->EndFrame();
}
```

### 9.2 性能优化建议

1. **减少RenderPass切换**：每次BeginRenderPass都会创建新encoder，有性能开销。
2. **合并Draw Call**：使用实例化渲染（DrawArraysInstanced）。
3. **复用RenderPass配置**：避免每帧重新创建RenderPassMTL对象。
4. **异步资源加载**：利用Metal的多线程能力。
5. **避免CPU/GPU同步**：不要在渲染循环中调用WaitIdle。

---

## 10. 总结

### 10.1 Metal后端优势

- ✅ **显式控制**：渲染流程清晰，易于理解和调试
- ✅ **高性能**：原生支持多线程，GPU资源管理更高效
- ✅ **现代化设计**：支持MRT、计算着色器等高级特性
- ✅ **跨平台**：同时支持macOS和iOS

### 10.2 关键要点

1. **必须显式管理RenderPass**：不能像OpenGL那样"随时"绘制
2. **CommandBuffer是一次性的**：不能复用，使用池化管理并发
3. **状态栈支持嵌套渲染**：自动保存/恢复视口等状态
4. **帧级CommandBuffer模式**：避免资源泄漏和管理混乱
5. **超时机制保证程序能正常退出**：避免waitUntilCompleted阻塞

### 10.3 进一步学习

- [Apple Metal文档](https://developer.apple.com/metal/)
- [Metal最佳实践指南](https://developer.apple.com/documentation/metal/best_practices)
- [LREngine示例代码](../examples/)
  - `HelloTriangleMTL` - 基础三角形渲染
  - `RenderToTextureMTL` - 离屏渲染和MRT
  - `TexturedCubeMTL` - 纹理和深度测试

---

**文档版本**: 1.0  
**维护者**: LREngine团队  
**最后更新**: 2026-01-18
