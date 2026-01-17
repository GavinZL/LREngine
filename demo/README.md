# LREngine iOS Demo

这是一个演示如何在iOS应用中集成LREngine图形引擎的示例项目。

## 功能特性

- ✅ 使用LREngine Metal后端渲染3D图形
- ✅ 显示旋转的3D立方体
- ✅ 实时渲染循环
- ✅ 支持Retina显示屏
- ✅ 响应式布局（支持旋转和各种屏幕尺寸）
- ✅ Objective-C++桥接层封装C++接口供Swift调用

## 项目结构

```
demo/
└── LREngine/
    ├── LREngine.xcodeproj/          # Xcode项目文件
    ├── LREngine/                    # 源代码目录
    │   ├── AppDelegate.swift        # 应用委托
    │   ├── SceneDelegate.swift      # 场景委托
    │   ├── ViewController.swift     # 主视图控制器（渲染立方体）
    │   ├── LREngineBridge.h         # Objective-C++桥接头文件
    │   ├── LREngineBridge.mm        # Objective-C++桥接实现
    │   ├── LREngine-Bridging-Header.h # Swift桥接头文件
    │   ├── Info.plist               # 应用配置
    │   ├── Base.lproj/              # 界面资源
    │   └── Assets.xcassets/         # 资源目录
    ├── iOS_Demo_Setup_Guide.md      # 详细配置指南
    └── README.md                    # 本文件
```

## 快速开始

### 1. 编译LREngine库

在运行demo之前，需要先编译LREngine的iOS版本：

```bash
cd /Volumes/LiSSD/ProjectT/MyProject/github/LREngine
./script/build_ios.sh -c Debug
```

### 2. 配置Xcode项目

请参考 [iOS_Demo_Setup_Guide.md](./LREngine/iOS_Demo_Setup_Guide.md) 进行详细配置。

主要配置步骤：
1. 添加LREngine静态库和系统框架
2. 设置头文件搜索路径
3. 配置Swift桥接头文件
4. 配置C++编译选项

### 3. 运行项目

1. 在Xcode中打开 `LREngine.xcodeproj`
2. 选择目标设备（iOS模拟器或真机）
3. 点击运行按钮 (⌘R)

## 技术架构

### 架构层次

```
┌─────────────────────────────────────┐
│         Swift UI Layer              │  ViewController.swift
│         (视图控制器)                 │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│    Objective-C++ Bridge Layer       │  LREngineBridge.h/.mm
│    (C++/Swift桥接层)                 │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│      LREngine C++ Library           │  liblrengine.a
│      (图形引擎核心)                   │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│        Metal Framework              │  系统框架
│        (iOS图形API)                  │
└─────────────────────────────────────┘
```

### 关键组件

#### 1. ViewController (Swift)
- 创建和管理Metal视图层
- 驱动渲染循环（使用CADisplayLink）
- 处理视图生命周期和布局变化
- 调用Objective-C++桥接层进行渲染

#### 2. LREngineBridge (Objective-C++)
- 封装LREngine C++ API
- 管理渲染资源（缓冲区、着色器、管线状态）
- 计算MVP矩阵和动画
- 提供Swift友好的接口

#### 3. LREngine Library (C++)
- Metal后端渲染实现
- 资源管理和生命周期控制
- 跨平台抽象层

## 渲染管线

```
1. ViewController.renderFrame()
   ↓
2. LREngineRenderer.renderFrame(deltaTime)
   ↓
3. 更新Uniform数据（MVP矩阵）
   ↓
4. LRRenderContext.BeginFrame()
   ↓
5. 清除屏幕
   ↓
6. 绑定管线状态和资源
   ↓
7. 绘制立方体（36个顶点）
   ↓
8. LRRenderContext.EndFrame()
   ↓
9. Metal呈现到屏幕
```

## 立方体渲染细节

### 几何数据
- **顶点数**：36（6个面 × 每面2个三角形 × 每三角形3个顶点）
- **顶点格式**：位置(Vec3) + 纹理坐标(Vec2) + 法线(Vec3)
- **总大小**：36 × 32字节 = 1152字节

### MVP变换
- **模型矩阵**：绕Y轴和X轴旋转
- **视图矩阵**：相机位置(0, 0, 3)，看向原点(0, 0, 0)
- **投影矩阵**：透视投影，FOV=45°，近平面=0.1，远平面=100

### 渲染状态
- **图元类型**：三角形
- **深度测试**：启用（CompareFunc::Less）
- **面剔除**：背面剔除
- **正面朝向**：逆时针（CCW）

## 性能特性

- 使用Metal硬件加速渲染
- 双缓冲（Metal自动管理）
- VSync同步（可配置）
- 高效的资源管理（静态顶点缓冲区）
- 动态Uniform更新（每帧更新MVP矩阵）

## 自定义和扩展

### 修改立方体颜色

编辑 `LREngineBridge.mm` 中的片段着色器：

```cpp
fragment float4 fragmentMain(VertexOut in [[stage_in]]) {
    // 修改这里的颜色计算
    float3 baseColor = float3(1.0, 0.5, 0.2); // 橙色
    // ...
}
```

### 添加纹理

1. 在 `LREngineBridge.mm` 中添加纹理加载代码
2. 创建 `LRTexture` 对象
3. 在渲染循环中调用 `context->SetTexture(texture, 0)`
4. 修改片段着色器进行纹理采样

参考：`examples/TexturedCubeMTL/main.mm`

### 修改相机位置

在 `LREngineBridge.mm` 的 `renderFrame:` 方法中修改：

```cpp
Vec3f eye(0.0f, 2.0f, 5.0f);  // 相机位置
Vec3f center(0.0f, 0.0f, 0.0f);  // 观察点
```

### 添加触摸交互

在 `ViewController.swift` 中添加手势识别器：

```swift
override func viewDidLoad() {
    super.viewDidLoad()
    
    let panGesture = UIPanGestureRecognizer(target: self, action: #selector(handlePan))
    metalView.addGestureRecognizer(panGesture)
}
```

## 系统要求

- **iOS版本**：13.0+
- **Xcode版本**：14.0+
- **Metal支持**：所有支持Metal的iOS设备
- **架构**：arm64（真机）/ x86_64（模拟器）

## 故障排除

### 黑屏问题
- 检查是否成功创建Metal设备
- 查看Xcode控制台的日志输出
- 确认LREngine库正确链接

### 编译错误
- 参考 [iOS_Demo_Setup_Guide.md](./LREngine/iOS_Demo_Setup_Guide.md)
- 确认所有搜索路径正确配置
- 检查C++标准设置为C++17

### 性能问题
- 使用Release配置编译LREngine
- 在真机上测试（模拟器性能较低）
- 检查是否启用了VSync

## 相关文档

- [iOS构建脚本文档](../script/iOS_Integration_Guide.md)
- [LREngine架构文档](../Doc/LREngine_Documentation.md)
- [Metal平台分析](../Doc/Metal_Platform_Compatibility_Analysis.md)

## 许可证

本示例代码遵循LREngine项目的许可证。

## 联系方式

如有问题或建议，请参考LREngine主项目的贡献指南。
