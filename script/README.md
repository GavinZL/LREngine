# LREngine iOS 构建脚本说明

本目录包含用于构建LREngine iOS库的自动化脚本和文档。

## 📁 目录内容

### 脚本文件

| 文件 | 大小 | 说明 |
|------|------|------|
| `build_ios.sh` | 20KB | **主构建脚本** - 用于编译iOS静态库和Framework |
| `verify_ios_build.sh` | 5.3KB | **验证脚本** - 验证构建输出的完整性和正确性 |

### 文档文件

| 文件 | 大小 | 说明 |
|------|------|------|
| `iOS_Quick_Reference.md` | 5.6KB | **快速参考** - 常用命令和快速入门指南 |
| `iOS_Integration_Guide.md` | 12KB | **完整集成指南** - 详细的集成步骤和示例代码 |
| `README.md` | - | **本文件** - 脚本目录说明 |

## 🚀 快速开始

### 1. 构建库

```bash
# 进入script目录
cd script

# 构建Release版本（推荐）
./build_ios.sh

# 或构建Debug版本
./build_ios.sh -c Debug
```

### 2. 验证构建

```bash
# 验证构建输出
./verify_ios_build.sh
```

### 3. 集成到项目

参考 [`iOS_Integration_Guide.md`](iOS_Integration_Guide.md) 获取详细集成步骤。

## 📋 主要功能

### build_ios.sh

**核心功能：**
- ✅ 为iOS设备（arm64架构）编译LREngine库
- ✅ 生成静态库（.a文件）
- ✅ 生成Framework（.framework包）
- ✅ 支持Debug和Release配置
- ✅ 自动配置Metal后端
- ✅ 链接必要的系统框架（Metal、Foundation、QuartzCore、UIKit等）
- ✅ 自动生成头文件和使用说明

**命令行选项：**
```
-c, --config <Debug|Release>        构建配置（默认：Release）
-t, --type <static|framework|all>   输出类型（默认：all）
-o, --output <路径>                 输出目录（默认：./build/ios）
-h, --help                          显示帮助信息
```

**构建输出：**
```
build/ios/
├── lib/
│   └── liblrengine.a              # 静态库
├── framework/
│   └── LREngine.framework/        # Framework包
├── include/
│   └── lrengine/                  # 公共头文件
└── README.md                      # 自动生成的使用说明
```

### verify_ios_build.sh

**验证内容：**
- ✅ 静态库是否存在
- ✅ Framework结构是否完整
- ✅ 架构支持（arm64）
- ✅ 关键符号完整性
- ✅ 头文件完整性
- ✅ Info.plist配置正确性

## 📖 使用文档

### 快速参考

查看 [`iOS_Quick_Reference.md`](iOS_Quick_Reference.md) 获取：
- 常用构建命令
- 输出文件结构说明
- 快速集成步骤
- 常见问题解决

### 完整集成指南

查看 [`iOS_Integration_Guide.md`](iOS_Integration_Guide.md) 获取：
- 详细的Xcode项目配置步骤
- 完整的示例代码
- 三角形渲染示例
- 常见问题详解

## 🔧 系统要求

### 构建环境
- **操作系统**: macOS 10.15+
- **Xcode**: 13.0+
- **CMake**: 3.15+
- **命令行工具**: xcode-select

### 目标平台
- **iOS版本**: 13.0+
- **架构**: arm64（仅真机）
- **图形API**: Metal
- **语言标准**: C++17

## 💡 常用场景

### 场景1: 首次构建

```bash
cd script
./build_ios.sh
./verify_ios_build.sh
```

### 场景2: 开发调试

```bash
# 构建Debug版本用于开发
./build_ios.sh -c Debug -t framework
```

### 场景3: 发布版本

```bash
# 构建优化的Release版本
./build_ios.sh -c Release -t all
```

### 场景4: 只需静态库

```bash
# 快速构建静态库
./build_ios.sh -t static
```

## 🏗️ 技术细节

### CMake配置

脚本自动设置以下CMake选项：

```cmake
-DCMAKE_SYSTEM_NAME=iOS                     # iOS平台
-DCMAKE_OSX_DEPLOYMENT_TARGET=13.0          # 最低iOS版本
-DCMAKE_OSX_ARCHITECTURES=arm64             # 架构
-DLRENGINE_ENABLE_METAL=ON                  # 启用Metal
-DLRENGINE_ENABLE_OPENGL=OFF                # 禁用OpenGL
-DLRENGINE_BUILD_EXAMPLES=OFF               # 不构建示例
-DLRENGINE_BUILD_TESTS=OFF                  # 不构建测试
```

### 系统框架链接

自动链接以下iOS系统框架：
- `Metal.framework` - Metal图形API
- `MetalKit.framework` - Metal工具包
- `QuartzCore.framework` - 核心动画和渲染
- `Foundation.framework` - 基础框架
- `UIKit.framework` - iOS UI框架

### 编译标志

- **ARC**: 自动引用计数（`-fobjc-arc`）
- **标准**: C++17
- **优化**: Release启用，Debug禁用
- **代码签名**: 构建时禁用（用户集成时配置）

## 📊 构建性能

| 配置 | 静态库 | Framework | 总时间 |
|------|--------|-----------|--------|
| Release | ~30s | ~45s | ~1-2分钟 |
| Debug | ~25s | ~35s | ~1分钟 |

*时间基于MacBook Pro (M1)，实际时间可能因硬件而异*

## 🐛 故障排除

### 常见问题

**问题1: CMake未找到**
```bash
brew install cmake
```

**问题2: iOS SDK未找到**
```bash
xcode-select --install
```

**问题3: 构建失败**
```bash
# 清理缓存重试
rm -rf ../build/ios/build_*
./build_ios.sh
```

**问题4: 验证失败**
```bash
# 查看详细错误信息
./verify_ios_build.sh
```

### 获取帮助

```bash
# 查看构建脚本帮助
./build_ios.sh --help

# 查看集成指南
cat iOS_Integration_Guide.md

# 查看快速参考
cat iOS_Quick_Reference.md
```

## 📝 脚本特性

### 用户友好性
- ✅ 彩色输出，清晰区分信息、成功、警告和错误
- ✅ 详细的进度提示
- ✅ 完整的帮助信息
- ✅ 自动环境检查

### 健壮性
- ✅ 参数验证
- ✅ 环境检查（CMake、Xcode、iOS SDK）
- ✅ 错误时自动退出（`set -e`）
- ✅ 输出验证

### 自动化
- ✅ 自动清理旧构建
- ✅ 自动创建目录结构
- ✅ 自动生成Framework结构
- ✅ 自动生成Info.plist
- ✅ 自动生成模块映射
- ✅ 自动生成使用说明

### 灵活性
- ✅ 支持Debug/Release配置
- ✅ 支持多种输出类型
- ✅ 可自定义输出目录
- ✅ 独立构建各类型

## 🔄 版本历史

### v1.0.0 (2026-01-17)
- ✨ 初始版本
- ✅ 支持iOS arm64架构
- ✅ 静态库构建
- ✅ Framework构建
- ✅ Debug/Release配置
- ✅ 自动验证
- ✅ 完整文档

## 📞 技术支持

遇到问题？

1. **查看文档**:
   - [快速参考](iOS_Quick_Reference.md)
   - [集成指南](iOS_Integration_Guide.md)
   
2. **查看项目文档**:
   - [LREngine文档](../Doc/LREngine_Documentation.md)
   - [Metal兼容性分析](../Doc/Metal_Platform_Compatibility_Analysis.md)

3. **查看示例**:
   - [示例代码](../examples/)

## 📄 许可证

遵循LREngine项目的许可证。

---

**脚本维护**: LREngine开发团队  
**最后更新**: 2026-01-17  
**版本**: 1.0.0
