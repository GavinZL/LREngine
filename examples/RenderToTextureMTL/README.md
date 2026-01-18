# RenderToTextureMTL Example

## 概述

这个示例演示了如何使用 LREngine 的 Metal 后端实现**离屏渲染（Render-to-Texture）**和**多通道渲染（Multi-pass Rendering）**。

## 功能特性

### 1. 离屏渲染（Offscreen Rendering）
- 创建离屏渲染目标（FrameBuffer）
- 渲染到纹理而不是直接到屏幕
- 使用自定义的颜色和深度附件

### 2. 多个3D立方体渲染
- 绘制三个具有不同变换的立方体：
  - **左侧立方体**：位于 (-2.5, 0, 0)，绕 Y 轴旋转
  - **中间立方体**：位于原点，绕 X 轴旋转，放大 1.3 倍
  - **右侧立方体**：位于 (2.5, 0, 0)，绕 Z 轴旋转，缩小至 0.7 倍
- 所有立方体使用相同的纹理贴图
- 应用简单的光照计算

### 3. 全屏纹理显示
- 将离屏渲染的结果纹理全屏显示
- 使用全屏四边形（Quad）映射纹理到屏幕

### 4. 多通道渲染流程
```
Pass 1: 渲染立方体 → 离屏纹理
  ├── BeginRenderPass(offscreenFrameBuffer)
  ├── 绘制立方体1
  ├── 绘制立方体2
  ├── 绘制立方体3
  └── EndRenderPass()

Pass 2: 全屏显示 → 屏幕
  ├── BeginRenderPass(nullptr)  // 默认帧缓冲
  ├── 绘制全屏Quad（使用离屏纹理）
  └── EndRenderPass()
```

## 技术要点

### 离屏渲染资源
- **颜色纹理**：RGBA8 格式，1024x768
- **深度纹理**：Depth32F 格式，1024x768
- **FrameBuffer**：绑定颜色和深度纹理作为渲染目标

### 着色器
- **Cube Shader**：标准的 MVP 变换 + 纹理采样 + 简单光照
- **Quad Shader**：简单的纹理采样，直接输出到屏幕

### 渲染管线
- **Cube Pipeline**：启用深度测试、背面剔除
- **Quad Pipeline**：禁用深度测试、无剔除

## 运行示例

```bash
cd /Volumes/LiSSD/ProjectT/MyProject/github/LREngine
cmake -S . -B build -DLRENGINE_ENABLE_METAL=ON
cmake --build build --target RenderToTextureMTL
./build/bin/examples/RenderToTextureMTL
```

## 应用场景

这个技术可以用于：
- **后处理效果**：模糊、色调映射、辉光等
- **镜面反射**：渲染反射内容到纹理
- **阴影贴图**：从光源视角渲染深度到纹理
- **小地图**：从俯视角度渲染场景到纹理
- **画中画**：在主场景中显示另一个视角
- **UI显示**：将3D内容渲染到UI纹理

## 代码结构

```
main.mm
├── 初始化渲染上下文
├── 创建Cube渲染资源
│   ├── 顶点/片段着色器
│   ├── 顶点缓冲区
│   ├── 纹理
│   └── 管线状态
├── 创建Quad渲染资源
│   ├── 顶点/片段着色器
│   ├── 顶点缓冲区
│   └── 管线状态
├── 创建离屏渲染目标
│   ├── 颜色纹理
│   ├── 深度纹理
│   └── FrameBuffer
└── 主循环
    ├── Pass 1: 离屏渲染
    └── Pass 2: 全屏显示
```

## 注意事项

1. **纹理格式**：确保离屏纹理格式与 FrameBuffer 配置匹配
2. **纹理采样器**：离屏纹理使用 `ClampToEdge` 避免边缘采样问题
3. **深度测试**：第一个 Pass 需要深度测试，第二个 Pass 不需要
4. **渲染顺序**：必须先完成离屏渲染，再使用结果纹理
5. **资源清理**：按照创建的逆序删除资源

## 参考

- [TexturedCubeMTL](../TexturedCubeMTL/main.mm) - 基础的立方体纹理渲染
- [LRFrameBuffer.h](../../include/lrengine/core/LRFrameBuffer.h) - FrameBuffer API
- [LRRenderContext.h](../../include/lrengine/core/LRRenderContext.h) - 渲染上下文 API
