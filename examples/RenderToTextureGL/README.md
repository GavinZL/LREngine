# RenderToTextureGL 示例

## 概述

本示例演示了如何使用 LREngine 的 OpenGL 后端实现离屏渲染（Render-to-Texture）技术。这是与 [RenderToTextureMTL](../RenderToTextureMTL) 示例功能完全相同的 OpenGL 版本实现。

## 功能特性

1. **离屏渲染（Offscreen Rendering）**
   - 创建自定义帧缓冲对象（Framebuffer Object, FBO）
   - 将渲染结果输出到纹理而非屏幕
   - 支持颜色附件和深度附件

2. **多 Pass 渲染流程**
   - **Pass 1**: 将三个立方体渲染到离屏纹理
   - **Pass 2**: 将离屏纹理渲染到全屏四边形，显示在屏幕上

3. **三个不同的立方体变换**
   - 左侧立方体：平移 + Y轴旋转
   - 中间立方体：缩放(1.3x) + X轴旋转(1.5倍速)
   - 右侧立方体：平移 + 缩放(0.7x) + Z轴旋转(2倍速)

4. **完整的 OpenGL 渲染管线**
   - 顶点缓冲区和顶点布局配置
   - GLSL 着色器程序（顶点 + 片段着色器）
   - 纹理加载和采样
   - 深度测试和面剔除配置
   - 简单的 Phong 光照模型

## 技术要点

### 离屏渲染纹理配置
```cpp
TextureDescriptor offscreenColorDesc;
offscreenColorDesc.type = TextureType::Texture2D;
offscreenColorDesc.width = OFFSCREEN_WIDTH;
offscreenColorDesc.height = OFFSCREEN_HEIGHT;
offscreenColorDesc.format = PixelFormat::RGBA8;
offscreenColorDesc.data = nullptr;  // 离屏纹理无需初始数据
```

### FrameBuffer 配置
```cpp
FrameBufferDescriptor fbDesc;
fbDesc.width = OFFSCREEN_WIDTH;
fbDesc.height = OFFSCREEN_HEIGHT;
fbDesc.hasDepthStencil = true;

// 颜色附件
ColorAttachmentDescriptor colorAttachment;
colorAttachment.format = PixelFormat::RGBA8;
colorAttachment.loadOp = LoadOp::Clear;
colorAttachment.storeOp = StoreOp::Store;
fbDesc.colorAttachments.push_back(colorAttachment);

// 深度附件
fbDesc.depthStencilAttachment.format = PixelFormat::Depth32F;
```

### 多 Pass 渲染流程
```cpp
// Pass 1: 渲染到离屏纹理
context->Clear(ClearFlag_Color | ClearFlag_Depth, ...);
context->BeginRenderPass(offscreenFrameBuffer);
// 绘制三个立方体
context->EndRenderPass();

// Pass 2: 渲染到屏幕
context->Clear(ClearFlag_Color, ...);
context->BeginRenderPass(nullptr);  // nullptr 表示默认帧缓冲
// 绘制全屏四边形，使用离屏纹理
context->EndRenderPass();
```

## 着色器说明

### Cube 着色器
- **顶点着色器**: 实现 MVP 变换，传递纹理坐标、法线和世界坐标
- **片段着色器**: 纹理采样 + 简单方向光照（环境光 + 漫反射）

### Quad 着色器
- **顶点着色器**: 直接传递屏幕空间坐标和纹理坐标
- **片段着色器**: 简单的纹理采样，用于显示离屏渲染结果

## 构建和运行

### 构建
```bash
cd /path/to/LREngine
cmake -B build -DLRENGINE_ENABLE_OPENGL=ON
cmake --build build --target RenderToTextureGL
```

### 运行
```bash
./build/bin/examples/RenderToTextureGL
```

## 与 Metal 版本的对比

| 特性 | OpenGL 版本 | Metal 版本 |
|------|------------|-----------|
| 着色器语言 | GLSL 3.3 | MSL (Metal Shading Language) |
| 帧缓冲创建 | OpenGL FBO | MTLTexture + MTLRenderPassDescriptor |
| Uniform 更新 | SetUniformMatrix4() | UpdateData() 到 UniformBuffer |
| 纹理坐标 | 左下角为原点 (0,0) | 左上角为原点 (0,0) |
| 面剔除默认方向 | CCW | CCW |

## 应用场景

1. **后处理效果**: 在纹理上应用模糊、色调映射等效果
2. **阴影映射**: 从光源视角渲染深度纹理
3. **反射/镜面**: 渲染场景的镜像版本
4. **多通道渲染**: 延迟渲染（Deferred Rendering）
5. **画中画效果**: 在场景中显示另一个视角的渲染结果

## 注意事项

1. **纹理格式**: 确保颜色和深度纹理的格式与 FrameBuffer 附件配置一致
2. **视口设置**: 离屏渲染时，视口尺寸应与离屏纹理尺寸匹配
3. **纹理采样**: 离屏纹理的采样器应设置为 `ClampToEdge` 避免边缘问题
4. **资源清理**: 及时释放 FrameBuffer、纹理等资源

## 参考资料

- [OpenGL Framebuffer Objects](https://www.khronos.org/opengl/wiki/Framebuffer_Object)
- [Learn OpenGL - Framebuffers](https://learnopengl.com/Advanced-OpenGL/Framebuffers)
- LREngine Metal 版本: [RenderToTextureMTL](../RenderToTextureMTL)
