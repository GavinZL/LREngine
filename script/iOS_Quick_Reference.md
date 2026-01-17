# LREngine iOS 构建快速参考

## 🚀 快速开始

```bash
# 进入脚本目录
cd script

# 构建Release版本（静态库 + Framework）
./build_ios.sh

# 验证构建
./verify_ios_build.sh
```

## 📦 构建命令

### 基本构建

```bash
# 默认构建（Release，所有类型）
./build_ios.sh

# Debug版本
./build_ios.sh -c Debug

# Release版本（显式指定）
./build_ios.sh -c Release
```

### 指定输出类型

```bash
# 只构建静态库
./build_ios.sh -t static

# 只构建Framework
./build_ios.sh -t framework

# 构建所有类型（默认）
./build_ios.sh -t all
```

### 自定义输出目录

```bash
# 指定输出目录
./build_ios.sh -o ~/Desktop/LREngine

# 组合使用
./build_ios.sh -c Debug -t framework -o ./custom_output
```

### 查看帮助

```bash
./build_ios.sh --help
```

## 📂 输出文件结构

构建完成后，输出目录结构如下：

```
build/ios/
├── lib/
│   └── liblrengine.a                 # 静态库
├── framework/
│   └── LREngine.framework/           # Framework包
│       ├── LREngine                  # 二进制文件
│       ├── Headers/                  # 头文件
│       │   ├── LREngine.h           # 伞形头文件
│       │   ├── core/                # 核心模块
│       │   ├── factory/             # 工厂类
│       │   ├── math/                # 数学库
│       │   └── utils/               # 工具类
│       ├── Info.plist               # Framework信息
│       └── module.modulemap         # 模块映射
├── include/
│   └── lrengine/                     # 公共头文件（静态库使用）
├── build_static_Release/             # 静态库构建缓存
├── build_framework_Release/          # Framework构建缓存
└── README.md                         # 使用说明
```

## 🔍 验证构建

```bash
# 验证构建输出
./verify_ios_build.sh
```

验证内容包括：
- ✓ 静态库是否存在
- ✓ 架构支持（arm64）
- ✓ 关键符号完整性
- ✓ Framework结构完整性
- ✓ 头文件完整性
- ✓ Info.plist配置

## 📋 系统要求

### 构建环境
- macOS系统
- Xcode命令行工具
- CMake 3.15+
- iOS SDK

### 目标平台
- iOS 13.0+
- arm64架构（仅真机）
- Metal图形API

## 💡 常用场景

### 场景1: 开发阶段（Debug）

```bash
# 构建Debug版本用于开发调试
./build_ios.sh -c Debug -t framework

# 验证
./verify_ios_build.sh
```

### 场景2: 发布版本（Release）

```bash
# 构建Release版本
./build_ios.sh -c Release -t all

# 验证
./verify_ios_build.sh
```

### 场景3: 快速迭代（只需静态库）

```bash
# 只构建静态库（更快）
./build_ios.sh -t static -c Debug
```

### 场景4: 清理重新构建

```bash
# 删除旧的构建目录
rm -rf build/ios

# 重新构建
./build_ios.sh
```

## 🔧 集成到Xcode项目

### 使用Framework（推荐）

1. 拖拽 `LREngine.framework` 到Xcode项目
2. 在Target设置中链接系统框架：
   - Metal.framework
   - MetalKit.framework
   - QuartzCore.framework
   - Foundation.framework
   - UIKit.framework
3. 在代码中导入：
   ```objc
   #import <LREngine/LREngine.h>
   ```

### 使用静态库

1. 添加 `liblrengine.a` 到项目
2. 配置Header Search Paths: `$(PROJECT_DIR)/include`
3. 链接系统框架（同上）
4. 在代码中导入：
   ```objc
   #import <lrengine/core/LRRenderContext.h>
   ```

## 📝 配置选项

### CMake配置

脚本自动配置以下CMake选项：

```cmake
-DCMAKE_SYSTEM_NAME=iOS                  # iOS平台
-DCMAKE_OSX_DEPLOYMENT_TARGET=13.0       # 最低支持iOS 13.0
-DCMAKE_OSX_ARCHITECTURES=arm64          # arm64架构
-DLRENGINE_ENABLE_METAL=ON               # 启用Metal后端
-DLRENGINE_ENABLE_OPENGL=OFF             # 禁用OpenGL（iOS不支持）
-DLRENGINE_BUILD_EXAMPLES=OFF            # 不构建示例
-DLRENGINE_BUILD_TESTS=OFF               # 不构建测试
```

### 编译标志

- **ARC**: 自动启用（`-fobjc-arc`）
- **标准**: C++17
- **优化**: Release模式启用，Debug模式禁用

## 🐛 故障排除

### 问题1: CMake未找到

```bash
# 安装CMake
brew install cmake
```

### 问题2: iOS SDK未找到

```bash
# 检查Xcode安装
xcode-select --print-path

# 重新安装命令行工具
xcode-select --install
```

### 问题3: 构建失败

```bash
# 清理构建缓存
rm -rf build/ios/build_*

# 重新构建
./build_ios.sh -c Debug
```

### 问题4: 架构不匹配

确保：
- 只为真机构建（arm64）
- 不支持模拟器（x86_64）
- 检查Xcode项目的架构设置

## 📖 更多文档

- **完整集成指南**: [iOS_Integration_Guide.md](iOS_Integration_Guide.md)
- **项目文档**: [../Doc/LREngine_Documentation.md](../Doc/LREngine_Documentation.md)
- **示例代码**: [../examples/](../examples/)
- **Metal兼容性**: [../Doc/Metal_Platform_Compatibility_Analysis.md](../Doc/Metal_Platform_Compatibility_Analysis.md)

## ⚡ 性能提示

1. **并行编译**: 脚本使用Xcode构建系统，自动并行编译
2. **增量构建**: 如果只修改了部分代码，构建缓存会加速编译
3. **清理缓存**: 如果遇到奇怪的问题，删除 `build/ios/build_*` 重新构建

## 🎯 最佳实践

1. **开发期间**: 使用Debug构建，便于调试
2. **性能测试**: 使用Release构建，启用优化
3. **版本控制**: 不要提交 `build/` 目录
4. **持续集成**: 可以在CI/CD中使用这些脚本自动构建

## 📞 获取帮助

```bash
# 查看构建脚本帮助
./build_ios.sh --help

# 查看完整集成指南
cat iOS_Integration_Guide.md
```

---

**版本**: 1.0.0  
**最后更新**: 2026-01-17
