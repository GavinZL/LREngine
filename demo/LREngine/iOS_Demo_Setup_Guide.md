# LREngine iOS Demo 配置指南

本文档说明如何配置iOS demo项目以集成LREngine库。

## 1. 前置准备

确保已经使用iOS构建脚本编译了LREngine库：

```bash
cd /Volumes/LiSSD/ProjectT/MyProject/github/LREngine
./script/build_ios.sh -c Debug
```

构建产物位置：
- 静态库：`build-ios/lib/liblrengine.a`
- Framework：`build-ios/framework/LREngine.framework`

## 2. Xcode项目配置步骤

### 2.1 添加LREngine静态库和头文件

1. 在Xcode中打开项目：`demo/LREngine/LREngine.xcodeproj`

2. 选择项目 -> `TARGETS` -> `LREngine` -> `Build Phases`

3. 展开 `Link Binary With Libraries`，点击`+`，添加：
   - `../../build-ios/lib/liblrengine.a`（选择"Add Other..." -> "Add Files..."）
   - `Metal.framework`
   - `MetalKit.framework`
   - `QuartzCore.framework`
   - `Foundation.framework`

### 2.2 配置头文件搜索路径

1. 选择 `Build Settings` 标签

2. 搜索 "Header Search Paths"

3. 添加以下路径（双击添加）：
   ```
   $(PROJECT_DIR)/../../include
   ```
   设置为 **recursive**（递归）

### 2.3 配置库文件搜索路径

1. 在 `Build Settings` 中搜索 "Library Search Paths"

2. 添加：
   ```
   $(PROJECT_DIR)/../../build-ios/lib
   ```

### 2.4 配置Swift桥接头文件

1. 在 `Build Settings` 中搜索 "Objective-C Bridging Header"

2. 设置值为：
   ```
   LREngine/LREngine-Bridging-Header.h
   ```

### 2.5 配置Objective-C++支持

1. 在 `Build Settings` 中搜索 "C++ Language Dialect"

2. 设置为：`GNU++17` 或 `C++17`

3. 搜索 "C++ Standard Library"

4. 设置为：`libc++ (LLVM C++ standard library with C++11 support)`

### 2.6 配置其他编译选项

1. 搜索 "Other C++ Flags"，添加：
   ```
   -DLRENGINE_ENABLE_METAL=1
   ```

2. 搜索 "Enable Bitcode"，设置为：`No`

### 2.7 配置部署目标

1. 确保 "iOS Deployment Target" >= `13.0`

## 3. 文件清单

确保以下文件已添加到项目：

### 必需文件
- ✅ `LREngine/LREngineBridge.h` - Objective-C++桥接头文件
- ✅ `LREngine/LREngineBridge.mm` - Objective-C++桥接实现
- ✅ `LREngine/LREngine-Bridging-Header.h` - Swift桥接头文件
- ✅ `LREngine/ViewController.swift` - 主视图控制器

### 现有文件
- `LREngine/AppDelegate.swift`
- `LREngine/SceneDelegate.swift`
- `LREngine/Info.plist`
- `Base.lproj/Main.storyboard`
- `Base.lproj/LaunchScreen.storyboard`
- `Assets.xcassets/`

## 4. 添加桥接文件到项目

如果文件未自动添加到项目：

1. 在Xcode左侧项目导航器中，右键点击 `LREngine` 文件夹

2. 选择 "Add Files to 'LREngine'..."

3. 找到并选择：
   - `LREngineBridge.h`
   - `LREngineBridge.mm`
   - `LREngine-Bridging-Header.h`

4. 确保：
   - [x] Copy items if needed
   - [x] Add to targets: LREngine

5. 点击 "Add"

## 5. 验证配置

### 5.1 检查编译设置

打开终端，运行以下命令检查配置：

```bash
cd /Volumes/LiSSD/ProjectT/MyProject/github/LREngine/demo/LREngine
xcodebuild -project LREngine.xcodeproj -target LREngine -configuration Debug -showBuildSettings | grep -E "(HEADER_SEARCH_PATHS|LIBRARY_SEARCH_PATHS|SWIFT_OBJC_BRIDGING_HEADER|CLANG_CXX_LANGUAGE_STANDARD)"
```

期望输出应包含：
```
HEADER_SEARCH_PATHS = ... ../../include ...
LIBRARY_SEARCH_PATHS = ... ../../build-ios/lib ...
SWIFT_OBJC_BRIDGING_HEADER = LREngine/LREngine-Bridging-Header.h
CLANG_CXX_LANGUAGE_STANDARD = gnu++17
```

### 5.2 清理并重新构建

在Xcode中：
1. 选择 `Product` -> `Clean Build Folder` (⇧⌘K)
2. 选择 `Product` -> `Build` (⌘B)

## 6. 运行项目

1. 选择iOS模拟器或真机作为运行目标

2. 点击运行按钮 (⌘R)

3. 应该看到一个旋转的3D立方体，背景为深蓝色

## 7. 常见问题

### 问题1: "liblrengine.a" 找不到

**解决方案**：
- 确保已运行 `build_ios.sh` 脚本
- 检查 `Library Search Paths` 是否正确
- 确认 `build-ios/lib/liblrengine.a` 文件存在

### 问题2: 头文件找不到

**解决方案**：
- 检查 `Header Search Paths` 是否包含 `$(PROJECT_DIR)/../../include`
- 确认路径设置为 **recursive**

### 问题3: Swift无法找到Objective-C类

**解决方案**：
- 检查 `Objective-C Bridging Header` 设置是否正确
- 确保 `LREngine-Bridging-Header.h` 正确导入了 `LREngineBridge.h`
- Clean build folder 后重新编译

### 问题4: C++编译错误

**解决方案**：
- 确认 `LREngineBridge.mm` 文件后缀为 `.mm`（不是 `.m`）
- 检查 C++ Language Dialect 设置为 C++17
- 确认 C++ Standard Library 设置为 libc++

### 问题5: 链接错误

**解决方案**：
- 确保所有必需的系统框架已添加（Metal, MetalKit, QuartzCore, Foundation）
- 检查是否正确链接了 `liblrengine.a`

## 8. 性能优化建议

1. **Release构建**：使用Release配置可获得更好的性能
   ```bash
   ./script/build_ios.sh -c Release
   ```

2. **真机测试**：在真实设备上测试可获得准确的性能数据

3. **分辨率调整**：如需提高性能，可在ViewController中调整渲染分辨率scale

## 9. 后续开发

### 添加纹理

修改 `LREngineBridge.mm` 中的着色器和资源创建代码，参考 `examples/TexturedCubeMTL/main.mm`

### 自定义几何体

修改 `cubeVertices` 数组来创建其他3D形状

### 交互控制

在 `ViewController.swift` 中添加触摸事件处理来控制相机或物体旋转

## 10. 更多资源

- LREngine文档：`/Volumes/LiSSD/ProjectT/MyProject/github/LREngine/Doc/`
- iOS示例源码：`/Volumes/LiSSD/ProjectT/MyProject/github/LREngine/examples/`
- 构建脚本：`/Volumes/LiSSD/ProjectT/MyProject/github/LREngine/script/`
