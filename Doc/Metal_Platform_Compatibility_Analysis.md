# Metal后端跨平台兼容性分析与实现方案

## 1. 概述

本文档详细分析了LREngine Metal后端在macOS和iOS平台之间的兼容性问题，并提供了完整的跨平台实现方案。

## 2. 平台差异分析

### 2.1 窗口系统差异

#### macOS平台
- **窗口类型**: `NSWindow`
- **视图类型**: `NSView`
- **框架**: Cocoa (`#import <Cocoa/Cocoa.h>`)
- **特性**:
  - 支持 `setWantsLayer:` 方法
  - 使用 `contentView` 获取内容视图
  - 支持 `backingScaleFactor` 处理Retina显示

#### iOS平台
- **窗口类型**: `UIWindow`
- **视图类型**: `UIView`
- **框架**: UIKit (`#import <UIKit/UIKit.h>`)
- **特性**:
  - 通过 `rootViewController.view` 访问视图
  - 使用 `UIScreen.mainScreen.nativeScale` 获取屏幕缩放
  - Layer直接添加到视图层次

### 2.2 Metal层配置差异

| 特性 | macOS | iOS |
|------|-------|-----|
| **设置方式** | `setLayer:` 替换整个layer | `addSublayer:` 添加到视图 |
| **缩放系数** | `backingScaleFactor` | `nativeScale` |
| **VSync控制** | `displaySyncEnabled` | iOS 13.0+ 支持 `displaySyncEnabled` |
| **存储模式** | 支持 `Managed` 模式 | 仅支持 `Shared` 和 `Private` |

### 2.3 纹理格式支持差异

#### 压缩格式

| 格式类型 | macOS | iOS |
|---------|-------|-----|
| **BC1-BC7** | ✓ 支持（桌面GPU） | ✗ 不支持 |
| **ASTC** | ✓ 支持（较新版本） | ✓ 支持（原生） |
| **ETC2** | ✗ 不支持 | ✓ 支持 |
| **PVRTC** | ✗ 不支持 | ✓ 支持 |

#### 深度格式

| 格式 | macOS | iOS |
|------|-------|-----|
| **Depth16Unorm** | ✓ | ✓ |
| **Depth32Float** | ✓ | ✓ |
| **Depth24Unorm_Stencil8** | ✓ | ✗ (回退到Depth32Float_Stencil8) |
| **Depth32Float_Stencil8** | ✓ | ✓ |

### 2.4 GPU Family差异

#### macOS
- `MTLGPUFamilyMac1` (macOS 10.14+)
- `MTLGPUFamilyMac2` (macOS 10.15+)
- `MTLGPUFamilyCommon1-3`

#### iOS
- `MTLGPUFamilyApple1-8` (不同iPhone/iPad代际)
- `MTLGPUFamilyCommon1-3`

## 3. 实现方案

### 3.1 条件编译策略

```objective-c++
// 平台检测宏
#if TARGET_OS_IPHONE || TARGET_OS_IOS
    // iOS特定代码
    #import <UIKit/UIKit.h>
#else
    // macOS特定代码
    #import <Cocoa/Cocoa.h>
#endif
```

### 3.2 窗口系统兼容实现

#### ContextMTL.mm 第81-96行改进

**修改前问题**:
- 仅支持 `NSWindow` 和 `NSView`（macOS专属）
- 无法在iOS平台编译

**修改后方案**:

```objective-c++
if (m_windowHandle) {
    #if TARGET_OS_IPHONE || TARGET_OS_IOS
        // iOS平台实现
        UIWindow* window = (__bridge UIWindow*)m_windowHandle;
        UIView* contentView = [window rootViewController].view;
        
        if (!contentView) {
            contentView = window;
        }
        
        // 配置Metal层
        m_metalLayer = [CAMetalLayer layer];
        m_metalLayer.device = m_device;
        m_metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        m_metalLayer.framebufferOnly = YES;
        
        // iOS使用nativeScale适配不同分辨率
        CGFloat scale = [UIScreen mainScreen].nativeScale;
        m_metalLayer.contentsScale = scale;
        m_metalLayer.drawableSize = CGSizeMake(m_width * scale, m_height * scale);
        
        // iOS 13.0+支持displaySyncEnabled
        #if defined(__IPHONE_13_0) && __IPHONE_OS_VERSION_MAX_ALLOWED >= __IPHONE_13_0
            if (@available(iOS 13.0, *)) {
                m_metalLayer.displaySyncEnabled = m_vsync;
            }
        #endif
        
        m_metalLayer.frame = contentView.bounds;
        [contentView.layer addSublayer:m_metalLayer];
        
    #else
        // macOS平台实现
        NSWindow* window = (__bridge NSWindow*)m_windowHandle;
        NSView* contentView = [window contentView];
        
        [contentView setWantsLayer:YES];
        m_metalLayer = [CAMetalLayer layer];
        m_metalLayer.device = m_device;
        m_metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        m_metalLayer.framebufferOnly = YES;
        m_metalLayer.drawableSize = CGSizeMake(m_width, m_height);
        m_metalLayer.displaySyncEnabled = m_vsync;
        m_metalLayer.contentsScale = contentView.window.backingScaleFactor;
        
        [contentView setLayer:m_metalLayer];
    #endif
}
```

### 3.3 纹理存储模式兼容

#### TextureMTL.mm 改进

```objective-c++
// 设置存储模式
#if TARGET_OS_IPHONE || TARGET_OS_IOS
    // iOS优先使用Shared模式
    texDesc.storageMode = MTLStorageModeShared;
#else
    // macOS支持Managed模式，更高效
    texDesc.storageMode = MTLStorageModeManaged;
#endif

// 深度纹理在所有平台都用Private
if (IsDepthFormat(desc.format)) {
    texDesc.usage |= MTLTextureUsageRenderTarget;
    texDesc.storageMode = MTLStorageModePrivate;
}
```

### 3.4 像素格式兼容

#### TypeConverterMTL.h 改进

```objective-c++
inline MTLPixelFormat ToMTLPixelFormat(PixelFormat format) {
    switch (format) {
        // 通用格式（所有平台）
        case PixelFormat::RGBA8: return MTLPixelFormatRGBA8Unorm;
        // ... 其他通用格式
        
        // 深度模板格式 - 平台特定
        case PixelFormat::Depth24Stencil8:
            #if TARGET_OS_IPHONE || TARGET_OS_IOS
                // iOS回退到Depth32Float_Stencil8
                return MTLPixelFormatDepth32Float_Stencil8;
            #else
                return MTLPixelFormatDepth24Unorm_Stencil8;
            #endif
        
        // BC压缩格式（仅macOS）
        #if !TARGET_OS_IPHONE && !TARGET_OS_IOS
        case PixelFormat::BC1: return MTLPixelFormatBC1_RGBA;
        // ... 其他BC格式
        #endif
        
        // ETC2压缩格式（仅iOS）
        #if TARGET_OS_IPHONE || TARGET_OS_IOS
        case PixelFormat::ETC2_RGB8: return MTLPixelFormatETC2_RGB8;
        #endif
        
        default: return MTLPixelFormatRGBA8Unorm;
    }
}
```

### 3.5 GPU能力检测兼容

#### FenceMTL.mm 改进

```objective-c++
bool FenceMTL::Create() {
    #if TARGET_OS_IPHONE || TARGET_OS_IOS
        // iOS使用Apple GPU Family
        if (@available(iOS 8.0, *)) {
            if (![m_device supportsFamily:MTLGPUFamilyApple1]) {
                m_signaled = false;
                return true;
            }
        }
    #else
        // macOS使用Mac GPU Family
        if (@available(macOS 10.14, *)) {
            if (![m_device supportsFamily:MTLGPUFamilyMac1]) {
                m_signaled = false;
                return true;
            }
        }
    #endif
    
    // 创建共享事件
    m_event = [m_device newSharedEvent];
    // ...
}
```

### 3.6 API版本兼容处理

#### ShaderMTL.mm 改进

```objective-c++
// 使用@available检查API可用性
if (@available(macOS 15.0, iOS 18.0, *)) {
    options.mathMode = MTLMathModeFast;  // 新API
} else {
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdeprecated-declarations"
    options.fastMathEnabled = YES;  // 旧API
    #pragma clang diagnostic pop
}
```

#### PipelineStateMTL.mm 改进

```objective-c++
// 采样数设置
if (@available(macOS 13.0, iOS 16.0, *)) {
    pipelineDesc.rasterSampleCount = desc.sampleCount;  // 新API
} else {
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdeprecated-declarations"
    pipelineDesc.sampleCount = desc.sampleCount;  // 旧API
    #pragma clang diagnostic pop
}
```

## 4. CMake配置更新

### 4.1 平台检测和定义

```cmake
# 平台检测
if(APPLE)
    set(LRENGINE_PLATFORM_APPLE TRUE)
    if(IOS)
        set(LRENGINE_PLATFORM_IOS TRUE)
    else()
        set(LRENGINE_PLATFORM_MACOS TRUE)
    endif()
endif()
```

### 4.2 Metal框架链接

```cmake
if(LRENGINE_ENABLE_METAL AND APPLE)
    target_link_libraries(lrengine PUBLIC
        "-framework Metal"
        "-framework MetalKit"
        "-framework QuartzCore"
        "-framework Foundation"
    )
    
    if(IOS)
        # iOS特定框架
        target_link_libraries(lrengine PUBLIC "-framework UIKit")
        target_compile_definitions(lrengine PUBLIC LRENGINE_PLATFORM_IOS)
    else()
        # macOS特定框架
        target_link_libraries(lrengine PUBLIC 
            "-framework Cocoa"
            "-framework IOKit"
        )
        target_compile_definitions(lrengine PUBLIC LRENGINE_PLATFORM_MACOS)
    endif()
    
    # 启用ARC
    set_source_files_properties(${LRENGINE_METAL_SOURCES} PROPERTIES
        COMPILE_FLAGS "-fobjc-arc"
    )
endif()
```

## 5. Metal最佳实践

### 5.1 资源管理

#### 内存管理
- **iOS**: 优先使用 `MTLStorageModeShared` 以减少内存占用
- **macOS**: 使用 `MTLStorageModeManaged` 获得更好性能
- **深度纹理**: 始终使用 `MTLStorageModePrivate`

#### 缓冲区使用
- 静态数据: `MTLResourceStorageModeShared`
- 动态数据: `MTLResourceStorageModeShared | MTLResourceCPUCacheModeWriteCombined`

### 5.2 命令缓冲区管理

```objective-c++
// 每帧创建新的命令缓冲区
id<MTLCommandBuffer> commandBuffer = [m_commandQueue commandBuffer];

// 使用完后立即提交
[commandBuffer commit];

// 等待完成（同步点）
[commandBuffer waitUntilCompleted];
```

### 5.3 渲染编码器生命周期

```objective-c++
// 确保在绘制前创建
void EnsureRenderEncoder() {
    if (m_currentRenderEncoder) return;
    
    // 确保有有效的drawable
    if (!m_currentDrawable && m_metalLayer) {
        m_currentDrawable = [m_metalLayer nextDrawable];
    }
    
    if (!m_currentDrawable) return;
    
    // 创建渲染通道描述符
    MTLRenderPassDescriptor* passDesc = [[MTLRenderPassDescriptor alloc] init];
    // ... 配置
    
    // 创建编码器
    m_currentRenderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:passDesc];
}

// 帧结束时销毁
void EndCurrentRenderEncoder() {
    if (m_currentRenderEncoder) {
        [m_currentRenderEncoder endEncoding];
        m_currentRenderEncoder = nil;
    }
}
```

### 5.4 性能优化建议

#### iOS特定优化
- 使用 `framebufferOnly = YES` 减少内存占用
- 避免频繁的CPU-GPU同步
- 使用 `MTLResourceStorageModeShared` 降低内存压力
- 利用Tile Memory优化（TBDR架构）

#### macOS特定优化
- 使用 `MTLResourceStorageModeManaged` 提升性能
- 充分利用独立显存（Discrete GPU）
- 使用 `MTLResourceCPUCacheModeWriteCombined` 优化写入

## 6. 兼容性测试矩阵

| 平台 | 最低版本 | 推荐版本 | Metal版本 |
|------|---------|---------|----------|
| macOS | 10.14 Mojave | 13.0+ | Metal 2+ |
| iOS | 8.0 | 16.0+ | Metal 1+ |
| iPadOS | 13.0 | 16.0+ | Metal 2+ |

## 7. 已知限制和解决方案

### 7.1 限制列表

| 限制 | 平台 | 解决方案 |
|------|------|---------|
| 不支持Depth24Stencil8 | iOS | 回退到Depth32Float_Stencil8 |
| 不支持BC压缩 | iOS | 使用ASTC或ETC2 |
| 不支持ETC2压缩 | macOS | 使用BC或ASTC |
| displaySyncEnabled | iOS <13.0 | 条件编译，旧版本不设置 |

### 7.2 回退策略

```objective-c++
// 格式回退示例
MTLPixelFormat SelectBestDepthFormat(bool needsStencil) {
    #if TARGET_OS_IPHONE || TARGET_OS_IOS
        return needsStencil ? 
            MTLPixelFormatDepth32Float_Stencil8 :
            MTLPixelFormatDepth32Float;
    #else
        return needsStencil ? 
            MTLPixelFormatDepth24Unorm_Stencil8 :
            MTLPixelFormatDepth32Float;
    #endif
}
```

## 8. 总结

通过以上跨平台兼容性改进，LREngine Metal后端现在支持：

✅ **macOS平台完整支持**
- NSWindow/NSView窗口系统
- 完整的Metal 2/3特性
- BC压缩纹理
- Managed存储模式

✅ **iOS平台完整支持**
- UIWindow/UIView窗口系统
- Metal 1/2/3特性
- ASTC/ETC2压缩纹理
- 移动GPU优化

✅ **统一API接口**
- 跨平台透明使用
- 自动平台检测和适配
- 优雅的降级策略

✅ **性能优化**
- 平台特定最佳实践
- 智能资源管理
- 高效的命令缓冲区处理

## 9. 参考资料

- [Metal Programming Guide - Apple Developer](https://developer.apple.com/metal/)
- [Metal Best Practices Guide](https://developer.apple.com/library/archive/documentation/Miscellaneous/Conceptual/MetalProgrammingGuide/)
- [Metal Feature Set Tables](https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf)
- [iOS GPU Family Comparison](https://developer.apple.com/documentation/metal/gpu_family)
