# LREngine iOS 集成指南

本指南详细说明如何在您的iOS项目中集成LREngine库。

## 📋 目录

1. [系统要求](#系统要求)
2. [构建LREngine](#构建lrengine)
3. [集成方式](#集成方式)
4. [示例代码](#示例代码)
5. [常见问题](#常见问题)

## 系统要求

### 开发环境
- macOS 10.15 或更高版本
- Xcode 13.0 或更高版本
- CMake 3.15 或更高版本

### 运行时要求
- iOS 13.0 或更高版本
- 支持Metal的iOS设备（iPhone 5s及以后设备）
- arm64架构

## 构建LREngine

### 1. 快速构建

在项目根目录下执行：

```bash
# 构建Release版本（包含静态库和Framework）
./script/build_ios.sh

# 构建Debug版本
./script/build_ios.sh -c Debug

# 只构建Framework
./script/build_ios.sh -t framework

# 只构建静态库
./script/build_ios.sh -t static
```

### 2. 构建选项说明

| 选项 | 说明 | 默认值 |
|------|------|--------|
| `-c, --config` | 构建配置（Debug/Release） | Release |
| `-t, --type` | 输出类型（static/framework/all） | all |
| `-o, --output` | 输出目录 | ./build/ios |
| `-h, --help` | 显示帮助信息 | - |

### 3. 验证构建

```bash
# 验证构建输出
./script/verify_ios_build.sh
```

## 集成方式

### 方式一：集成Framework（推荐）

#### 步骤1: 添加Framework

1. 在Xcode中打开您的iOS项目
2. 将构建好的 `LREngine.framework` 拖拽到项目导航器中
3. 确保勾选 "Copy items if needed"
4. 在目标(Target)设置中，找到 "Frameworks, Libraries, and Embedded Content"
5. 确认 `LREngine.framework` 已添加，并设置为 "Embed & Sign"

#### 步骤2: 链接系统框架

在 "Build Phases" → "Link Binary With Libraries" 中添加：

```
Metal.framework
MetalKit.framework
QuartzCore.framework
Foundation.framework
UIKit.framework
```

#### 步骤3: 配置编译选项

在 "Build Settings" 中：

1. **C++ Language Dialect**: 设置为 "GNU++17" 或 "C++17"
2. **Enable Bitcode**: 设置为 "No"（如果需要）

#### 步骤4: 在代码中使用

**Objective-C++文件（.mm）：**

```objc
#import <LREngine/LREngine.h>

@implementation MyViewController

- (void)setupRenderer {
    // 创建Metal渲染上下文
    auto context = LR::LRDeviceFactory::CreateRenderContext(LR::BackendAPI::Metal);
    
    // 获取Metal Layer
    CAMetalLayer* metalLayer = (CAMetalLayer*)self.metalView.layer;
    
    // 初始化上下文
    context->Initialize((__bridge void*)metalLayer);
    
    LR_LOG_INFO("LREngine初始化成功");
}

@end
```

**Swift（通过桥接头文件）：**

1. 创建桥接头文件 `YourProject-Bridging-Header.h`：

```objc
#import <LREngine/LREngine.h>
```

2. 在Swift代码中使用（需要通过Objective-C++包装器）

### 方式二：集成静态库

#### 步骤1: 添加库文件

1. 将 `liblrengine.a` 添加到项目中
2. 将 `include/lrengine/` 目录复制到项目中

#### 步骤2: 配置头文件搜索路径

在 "Build Settings" → "Header Search Paths" 中添加：

```
$(PROJECT_DIR)/include
```

设置为 **recursive**。

#### 步骤3: 链接库和框架

1. 在 "Build Phases" → "Link Binary With Libraries" 中添加 `liblrengine.a`
2. 添加系统框架（同Framework方式）

#### 步骤4: 配置其他链接标志

在 "Build Settings" → "Other Linker Flags" 中添加：

```
-ObjC
-lc++
```

#### 步骤5: 在代码中使用

```objc
#import <lrengine/core/LRRenderContext.h>
#import <lrengine/factory/LRDeviceFactory.h>
#import <lrengine/utils/LRLog.h>

// 使用方式同Framework
```

## 示例代码

### 创建Metal视图

```objc
// MetalView.h
#import <UIKit/UIKit.h>
#import <QuartzCore/CAMetalLayer.h>

@interface MetalView : UIView
@property (nonatomic, readonly) CAMetalLayer *metalLayer;
@end

// MetalView.mm
#import "MetalView.h"

@implementation MetalView

+ (Class)layerClass {
    return [CAMetalLayer class];
}

- (CAMetalLayer *)metalLayer {
    return (CAMetalLayer *)self.layer;
}

- (instancetype)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        self.metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    }
    return self;
}

@end
```

### 初始化渲染器

```objc
// Renderer.h
#import <Foundation/Foundation.h>
#import <QuartzCore/CAMetalLayer.h>

@interface Renderer : NSObject

- (instancetype)initWithMetalLayer:(CAMetalLayer *)layer;
- (void)render;

@end

// Renderer.mm
#import "Renderer.h"
#import <LREngine/LREngine.h>

@interface Renderer() {
    std::shared_ptr<LR::LRRenderContext> _context;
}
@end

@implementation Renderer

- (instancetype)initWithMetalLayer:(CAMetalLayer *)layer {
    if (self = [super init]) {
        // 创建渲染上下文
        _context = LR::LRDeviceFactory::CreateRenderContext(LR::BackendAPI::Metal);
        
        // 初始化
        if (!_context->Initialize((__bridge void*)layer)) {
            LR_LOG_ERROR("初始化渲染上下文失败");
            return nil;
        }
        
        // 设置视口
        LR::Viewport viewport;
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = layer.drawableSize.width;
        viewport.height = layer.drawableSize.height;
        _context->SetViewport(viewport);
        
        LR_LOG_INFO("渲染器初始化成功");
    }
    return self;
}

- (void)render {
    // 开始渲染
    _context->BeginFrame();
    
    // 清屏
    LR::ClearValue clearValue;
    clearValue.color = {0.0f, 0.5f, 1.0f, 1.0f};  // 蓝色背景
    _context->Clear(LR::ClearFlags::Color, clearValue);
    
    // 这里添加您的渲染命令
    
    // 结束渲染
    _context->EndFrame();
}

@end
```

### 在ViewController中使用

```objc
// ViewController.mm
#import "ViewController.h"
#import "MetalView.h"
#import "Renderer.h"

@interface ViewController ()
@property (nonatomic, strong) MetalView *metalView;
@property (nonatomic, strong) Renderer *renderer;
@property (nonatomic, strong) CADisplayLink *displayLink;
@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // 创建Metal视图
    self.metalView = [[MetalView alloc] initWithFrame:self.view.bounds];
    self.metalView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    [self.view addSubview:self.metalView];
    
    // 创建渲染器
    self.renderer = [[Renderer alloc] initWithMetalLayer:self.metalView.metalLayer];
    
    // 设置渲染循环
    self.displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(renderLoop)];
    [self.displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
}

- (void)renderLoop {
    [self.renderer render];
}

- (void)dealloc {
    [self.displayLink invalidate];
}

@end
```

### 创建三角形（完整示例）

```objc
// 在Renderer中添加三角形渲染

@interface Renderer() {
    std::shared_ptr<LR::LRRenderContext> _context;
    std::shared_ptr<LR::LRBuffer> _vertexBuffer;
    std::shared_ptr<LR::LRShader> _vertexShader;
    std::shared_ptr<LR::LRShader> _fragmentShader;
    std::shared_ptr<LR::LRPipelineState> _pipelineState;
}
@end

@implementation Renderer

- (instancetype)initWithMetalLayer:(CAMetalLayer *)layer {
    if (self = [super init]) {
        _context = LR::LRDeviceFactory::CreateRenderContext(LR::BackendAPI::Metal);
        
        if (!_context->Initialize((__bridge void*)layer)) {
            return nil;
        }
        
        [self setupTriangle];
    }
    return self;
}

- (void)setupTriangle {
    // 顶点数据
    struct Vertex {
        LR::Vec3 position;
        LR::Vec3 color;
    };
    
    Vertex vertices[] = {
        {{  0.0f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }},  // 顶部 - 红色
        {{ -0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }},  // 左下 - 绿色
        {{  0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }}   // 右下 - 蓝色
    };
    
    // 创建顶点缓冲
    LR::BufferDescriptor bufferDesc;
    bufferDesc.size = sizeof(vertices);
    bufferDesc.usage = LR::BufferUsage::Vertex;
    _vertexBuffer = _context->CreateBuffer(bufferDesc, vertices);
    
    // 顶点着色器（Metal Shading Language）
    const char* vertexShaderCode = R"(
        #include <metal_stdlib>
        using namespace metal;
        
        struct VertexIn {
            float3 position [[attribute(0)]];
            float3 color [[attribute(1)]];
        };
        
        struct VertexOut {
            float4 position [[position]];
            float3 color;
        };
        
        vertex VertexOut vertexShader(VertexIn in [[stage_in]]) {
            VertexOut out;
            out.position = float4(in.position, 1.0);
            out.color = in.color;
            return out;
        }
    )";
    
    // 片段着色器
    const char* fragmentShaderCode = R"(
        #include <metal_stdlib>
        using namespace metal;
        
        struct VertexOut {
            float4 position [[position]];
            float3 color;
        };
        
        fragment float4 fragmentShader(VertexOut in [[stage_in]]) {
            return float4(in.color, 1.0);
        }
    )";
    
    // 创建着色器
    LR::ShaderDescriptor vsDesc;
    vsDesc.type = LR::ShaderType::Vertex;
    vsDesc.source = vertexShaderCode;
    vsDesc.entryPoint = "vertexShader";
    _vertexShader = _context->CreateShader(vsDesc);
    
    LR::ShaderDescriptor fsDesc;
    fsDesc.type = LR::ShaderType::Fragment;
    fsDesc.source = fragmentShaderCode;
    fsDesc.entryPoint = "fragmentShader";
    _fragmentShader = _context->CreateShader(fsDesc);
    
    // 创建管线状态
    LR::PipelineStateDescriptor pipelineDesc;
    pipelineDesc.vertexShader = _vertexShader;
    pipelineDesc.fragmentShader = _fragmentShader;
    
    // 顶点布局
    LR::VertexAttribute posAttr;
    posAttr.format = LR::VertexFormat::Float3;
    posAttr.offset = 0;
    posAttr.location = 0;
    
    LR::VertexAttribute colorAttr;
    colorAttr.format = LR::VertexFormat::Float3;
    colorAttr.offset = sizeof(LR::Vec3);
    colorAttr.location = 1;
    
    LR::VertexBufferLayout layout;
    layout.stride = sizeof(Vertex);
    layout.attributes = { posAttr, colorAttr };
    
    pipelineDesc.vertexLayout.bufferLayouts = { layout };
    
    _pipelineState = _context->CreatePipelineState(pipelineDesc);
}

- (void)render {
    _context->BeginFrame();
    
    LR::ClearValue clearValue;
    clearValue.color = {0.2f, 0.2f, 0.2f, 1.0f};
    _context->Clear(LR::ClearFlags::Color, clearValue);
    
    // 设置管线状态
    _context->SetPipelineState(_pipelineState);
    
    // 绑定顶点缓冲
    _context->SetVertexBuffer(_vertexBuffer, 0, 0);
    
    // 绘制三角形
    _context->Draw(3, 1, 0, 0);
    
    _context->EndFrame();
}

@end
```

## 常见问题

### Q: 链接时出现符号未找到错误

**A:** 确保：
1. 已链接所有必需的系统框架
2. 在 "Other Linker Flags" 中添加了 `-ObjC` 和 `-lc++`
3. C++ Language Dialect 设置为 C++17

### Q: 运行时崩溃，提示Metal相关错误

**A:** 检查：
1. 设备是否支持Metal（模拟器不支持真实Metal）
2. CAMetalLayer是否正确配置
3. 是否在主线程初始化Metal相关对象

### Q: 无法在Swift中使用

**A:** LREngine是C++库，需要通过Objective-C++桥接使用：
1. 创建Objective-C++包装类（.mm文件）
2. 在桥接头文件中导入
3. 在Swift中使用包装类

### Q: Framework签名问题

**A:** 如果遇到签名问题：
1. 在 "Build Settings" 中搜索 "Code Signing"
2. 确保 "Code Signing Allowed" 为 YES
3. 选择合适的开发团队和证书

### Q: 编译速度慢

**A:** 优化建议：
1. 使用预编译头文件
2. 启用增量编译
3. 在 Debug 配置下关闭优化

### Q: 如何在模拟器上测试？

**A:** 当前构建仅支持真机（arm64），模拟器需要额外构建x86_64架构的版本。建议：
1. 真机测试渲染功能
2. 逻辑代码可以在模拟器上测试

### Q: 内存泄漏问题

**A:** LREngine使用智能指针管理内存，确保：
1. 使用 `std::shared_ptr` 管理LREngine对象
2. 在Objective-C++中正确管理对象生命周期
3. 避免循环引用

## 技术支持

如需更多帮助，请：

1. 查看项目文档：`Doc/LREngine_Documentation.md`
2. 查看示例代码：`examples/` 目录
3. 提交Issue到项目仓库

---

**文档版本**: 1.0.0  
**最后更新**: 2026年1月
