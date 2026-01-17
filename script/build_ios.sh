#!/bin/bash

################################################################################
# LREngine iOS 构建脚本
# 
# 功能：
# - 为iOS设备（arm64）编译LREngine库
# - 生成静态库（.a）和Framework（.framework）
# - 支持Debug和Release配置
# - 自动配置Metal后端
#
# 使用方法：
#   ./build_ios.sh [选项]
#
# 选项：
#   -c, --config <Debug|Release>  构建配置（默认：Release）
#   -t, --type <static|framework|all>  输出类型（默认：all）
#   -o, --output <路径>           输出目录（默认：./build/ios）
#   -h, --help                    显示帮助信息
#
# 示例：
#   ./build_ios.sh                          # 构建Release版本的静态库和Framework
#   ./build_ios.sh -c Debug                 # 构建Debug版本
#   ./build_ios.sh -t static                # 只构建静态库
#   ./build_ios.sh -t framework -c Debug    # 构建Debug版本的Framework
################################################################################

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 默认参数
BUILD_CONFIG="Release"
BUILD_TYPE="all"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
OUTPUT_DIR="${PROJECT_ROOT}/build/ios"
LIBRARY_NAME="LREngine"
VERSION="1.0.0"

# iOS部署目标版本
IOS_DEPLOYMENT_TARGET="13.0"

# 支持的架构（iOS设备仅使用arm64）
DEVICE_ARCHS="arm64"

################################################################################
# 辅助函数
################################################################################

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_header() {
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}$1${NC}"
    echo -e "${GREEN}========================================${NC}"
}

show_help() {
    cat << EOF
LREngine iOS 构建脚本

用法: $0 [选项]

选项:
  -c, --config <Debug|Release>        构建配置（默认：Release）
  -t, --type <static|framework|all>   输出类型（默认：all）
  -o, --output <路径>                 输出目录（默认：./build/ios）
  -h, --help                          显示此帮助信息

构建配置说明:
  Debug     - 包含调试符号，未优化
  Release   - 优化构建，移除调试符号

输出类型说明:
  static    - 只生成静态库（.a文件）
  framework - 只生成Framework（.framework包）
  all       - 同时生成静态库和Framework

示例:
  $0                                  # 构建Release版本的所有输出
  $0 -c Debug                         # 构建Debug版本
  $0 -t static                        # 只构建静态库
  $0 -t framework -c Debug            # 构建Debug版本的Framework
  $0 -o ~/Desktop/LREngine            # 输出到指定目录

输出文件结构:
  <输出目录>/
    ├── lib/
    │   └── liblrengine.a           # 静态库
    ├── framework/
    │   └── LREngine.framework/     # Framework包
    └── include/
        └── lrengine/               # 公共头文件

EOF
}

################################################################################
# 参数解析
################################################################################

while [[ $# -gt 0 ]]; do
    case $1 in
        -c|--config)
            BUILD_CONFIG="$2"
            shift 2
            ;;
        -t|--type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -o|--output)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            print_error "未知选项: $1"
            show_help
            exit 1
            ;;
    esac
done

# 验证构建配置
if [[ "$BUILD_CONFIG" != "Debug" && "$BUILD_CONFIG" != "Release" ]]; then
    print_error "无效的构建配置: $BUILD_CONFIG (必须是 Debug 或 Release)"
    exit 1
fi

# 验证构建类型
if [[ "$BUILD_TYPE" != "static" && "$BUILD_TYPE" != "framework" && "$BUILD_TYPE" != "all" ]]; then
    print_error "无效的构建类型: $BUILD_TYPE (必须是 static、framework 或 all)"
    exit 1
fi

################################################################################
# 环境检查
################################################################################

check_environment() {
    print_header "检查构建环境"
    
    # 检查是否在macOS上运行
    if [[ "$OSTYPE" != "darwin"* ]]; then
        print_error "此脚本只能在macOS上运行"
        exit 1
    fi
    
    # 检查CMake
    if ! command -v cmake &> /dev/null; then
        print_error "未找到CMake，请先安装CMake"
        print_info "可以使用 Homebrew 安装: brew install cmake"
        exit 1
    fi
    print_success "CMake 版本: $(cmake --version | head -n1)"
    
    # 检查Xcode命令行工具
    if ! command -v xcodebuild &> /dev/null; then
        print_error "未找到Xcode命令行工具"
        print_info "请运行: xcode-select --install"
        exit 1
    fi
    print_success "Xcode 版本: $(xcodebuild -version | head -n1)"
    
    # 检查iOS SDK
    IOS_SDK_PATH=$(xcrun --sdk iphoneos --show-sdk-path 2>/dev/null || echo "")
    if [[ -z "$IOS_SDK_PATH" ]]; then
        print_error "未找到iOS SDK"
        print_info "请确保已安装Xcode及iOS开发组件"
        exit 1
    fi
    print_success "iOS SDK 路径: $IOS_SDK_PATH"
    
    echo ""
}

################################################################################
# 清理函数
################################################################################

clean_build_dir() {
    local build_dir=$1
    if [[ -d "$build_dir" ]]; then
        print_info "清理旧的构建目录: $build_dir"
        rm -rf "$build_dir"
    fi
}

################################################################################
# 构建静态库
################################################################################

build_static_library() {
    print_header "构建iOS静态库 (${BUILD_CONFIG})"
    
    local build_dir="${OUTPUT_DIR}/build_static_${BUILD_CONFIG}"
    clean_build_dir "$build_dir"
    mkdir -p "$build_dir"
    
    print_info "配置CMake..."
    cmake -S "$PROJECT_ROOT" -B "$build_dir" \
        -G Xcode \
        -DCMAKE_SYSTEM_NAME=iOS \
        -DCMAKE_OSX_DEPLOYMENT_TARGET=${IOS_DEPLOYMENT_TARGET} \
        -DCMAKE_OSX_ARCHITECTURES="${DEVICE_ARCHS}" \
        -DCMAKE_BUILD_TYPE=${BUILD_CONFIG} \
        -DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=NO \
        -DCMAKE_IOS_INSTALL_COMBINED=YES \
        -DLRENGINE_ENABLE_METAL=ON \
        -DLRENGINE_ENABLE_OPENGL=OFF \
        -DLRENGINE_BUILD_EXAMPLES=OFF \
        -DLRENGINE_BUILD_TESTS=OFF \
        -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY="${OUTPUT_DIR}/lib" \
        -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO \
        -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED=NO
    
    print_info "编译静态库..."
    cmake --build "$build_dir" --config ${BUILD_CONFIG} --target lrengine -- -quiet
    
    # 复制静态库到输出目录
    local lib_output="${OUTPUT_DIR}/lib"
    mkdir -p "$lib_output"
    
    local static_lib="${build_dir}/lib/${BUILD_CONFIG}/liblrengine.a"
    if [[ -f "$static_lib" ]]; then
        cp "$static_lib" "$lib_output/liblrengine.a"
        print_success "静态库已生成: $lib_output/liblrengine.a"
        
        # 显示库信息
        print_info "库架构信息:"
        lipo -info "$lib_output/liblrengine.a"
        
        # 显示库大小
        local lib_size=$(du -h "$lib_output/liblrengine.a" | awk '{print $1}')
        print_info "库大小: $lib_size"
    else
        print_error "静态库构建失败"
        exit 1
    fi
    
    echo ""
}

################################################################################
# 构建Framework
################################################################################

build_framework() {
    print_header "构建iOS Framework (${BUILD_CONFIG})"
    
    local build_dir="${OUTPUT_DIR}/build_framework_${BUILD_CONFIG}"
    clean_build_dir "$build_dir"
    mkdir -p "$build_dir"
    
    print_info "配置CMake..."
    cmake -S "$PROJECT_ROOT" -B "$build_dir" \
        -G Xcode \
        -DCMAKE_SYSTEM_NAME=iOS \
        -DCMAKE_OSX_DEPLOYMENT_TARGET=${IOS_DEPLOYMENT_TARGET} \
        -DCMAKE_OSX_ARCHITECTURES="${DEVICE_ARCHS}" \
        -DCMAKE_BUILD_TYPE=${BUILD_CONFIG} \
        -DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=NO \
        -DCMAKE_IOS_INSTALL_COMBINED=YES \
        -DLRENGINE_ENABLE_METAL=ON \
        -DLRENGINE_ENABLE_OPENGL=OFF \
        -DLRENGINE_BUILD_EXAMPLES=OFF \
        -DLRENGINE_BUILD_TESTS=OFF \
        -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO \
        -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED=NO
    
    print_info "编译库文件..."
    cmake --build "$build_dir" --config ${BUILD_CONFIG} --target lrengine -- -quiet
    
    # 创建Framework结构
    local framework_dir="${OUTPUT_DIR}/framework/${LIBRARY_NAME}.framework"
    rm -rf "$framework_dir"
    mkdir -p "$framework_dir/Headers"
    
    # 复制静态库到Framework
    local static_lib="${build_dir}/lib/${BUILD_CONFIG}/liblrengine.a"
    if [[ ! -f "$static_lib" ]]; then
        print_error "未找到编译的静态库"
        exit 1
    fi
    
    cp "$static_lib" "$framework_dir/${LIBRARY_NAME}"
    
    # 复制公共头文件
    print_info "复制头文件到Framework..."
    cp -R "$PROJECT_ROOT/include/lrengine/"* "$framework_dir/Headers/"
    
    # 创建模块映射文件
    cat > "$framework_dir/Headers/module.modulemap" << EOF
framework module ${LIBRARY_NAME} {
    umbrella header "${LIBRARY_NAME}.h"
    export *
    module * { export * }
}
EOF
    
    # 创建伞形头文件
    cat > "$framework_dir/Headers/${LIBRARY_NAME}.h" << 'EOF'
//
//  LREngine.h
//  LREngine iOS Framework
//
//  自动生成的伞形头文件
//

#ifndef LRENGINE_H
#define LRENGINE_H

// 核心头文件
#import <LREngine/core/LRDefines.h>
#import <LREngine/core/LRTypes.h>
#import <LREngine/core/LRError.h>
#import <LREngine/core/LRResource.h>
#import <LREngine/core/LRBuffer.h>
#import <LREngine/core/LRShader.h>
#import <LREngine/core/LRTexture.h>
#import <LREngine/core/LRFrameBuffer.h>
#import <LREngine/core/LRPipelineState.h>
#import <LREngine/core/LRFence.h>
#import <LREngine/core/LRRenderContext.h>

// 工厂类
#import <LREngine/factory/LRDeviceFactory.h>

// 工具类
#import <LREngine/utils/LRLog.h>

// 数学库
#import <LREngine/math/MathFwd.hpp>
#import <LREngine/math/MathDef.hpp>
#import <LREngine/math/Vec2.hpp>
#import <LREngine/math/Vec3.hpp>
#import <LREngine/math/Vec4.hpp>
#import <LREngine/math/Mat3.hpp>
#import <LREngine/math/Mat4.hpp>
#import <LREngine/math/Quaternion.hpp>

#endif /* LRENGINE_H */
EOF
    
    # 创建Info.plist
    cat > "$framework_dir/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>en</string>
    <key>CFBundleExecutable</key>
    <string>${LIBRARY_NAME}</string>
    <key>CFBundleIdentifier</key>
    <string>com.lrengine.${LIBRARY_NAME}</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>${LIBRARY_NAME}</string>
    <key>CFBundlePackageType</key>
    <string>FMWK</string>
    <key>CFBundleShortVersionString</key>
    <string>${VERSION}</string>
    <key>CFBundleVersion</key>
    <string>${VERSION}</string>
    <key>CFBundleSupportedPlatforms</key>
    <array>
        <string>iPhoneOS</string>
    </array>
    <key>MinimumOSVersion</key>
    <string>${IOS_DEPLOYMENT_TARGET}</string>
</dict>
</plist>
EOF
    
    print_success "Framework已生成: $framework_dir"
    
    # 显示Framework信息
    print_info "Framework架构信息:"
    lipo -info "$framework_dir/${LIBRARY_NAME}"
    
    # 显示Framework大小
    local framework_size=$(du -sh "$framework_dir" | awk '{print $1}')
    print_info "Framework大小: $framework_size"
    
    echo ""
}

################################################################################
# 复制头文件
################################################################################

copy_headers() {
    print_info "复制公共头文件..."
    
    local include_dir="${OUTPUT_DIR}/include"
    rm -rf "$include_dir"
    mkdir -p "$include_dir"
    
    cp -R "$PROJECT_ROOT/include/lrengine" "$include_dir/"
    
    print_success "头文件已复制到: $include_dir"
}

################################################################################
# 生成使用说明
################################################################################

generate_usage_guide() {
    local usage_file="${OUTPUT_DIR}/README.md"
    
    cat > "$usage_file" << 'EOF'
# LREngine iOS 库使用指南

LREngine是一个轻量级跨平台渲染引擎，本文档说明如何在iOS项目中集成和使用LREngine。

## 📦 包含内容

构建完成后，您将获得以下文件：

```
ios/
├── lib/
│   └── liblrengine.a           # 静态库
├── framework/
│   └── LREngine.framework/     # Framework包
└── include/
    └── lrengine/               # 公共头文件（仅静态库使用时需要）
```

## 🔧 集成方式

### 方式一：使用Framework（推荐）

1. **添加Framework到项目**
   - 将 `LREngine.framework` 拖拽到Xcode项目中
   - 在项目设置的 "Frameworks, Libraries, and Embedded Content" 中确认Framework已添加
   - 选择 "Embed & Sign" 或 "Do Not Embed"（取决于您的需求）

2. **链接系统框架**
   
   在项目的 "Build Phases" → "Link Binary With Libraries" 中添加：
   - `Metal.framework`
   - `MetalKit.framework`
   - `QuartzCore.framework`
   - `Foundation.framework`
   - `UIKit.framework`

3. **在代码中使用**
   ```objc
   // Objective-C
   #import <LREngine/LREngine.h>
   ```
   
   ```swift
   // Swift（需要在桥接头文件中导入）
   // 在 YourProject-Bridging-Header.h 中：
   #import <LREngine/LREngine.h>
   ```

### 方式二：使用静态库

1. **添加静态库**
   - 将 `liblrengine.a` 添加到项目中
   - 将 `include/lrengine/` 目录添加到项目中

2. **配置头文件搜索路径**
   
   在 "Build Settings" → "Header Search Paths" 中添加：
   ```
   $(PROJECT_DIR)/include
   ```

3. **链接系统框架**
   
   同Framework方式，添加相同的系统框架。

4. **在代码中使用**
   ```objc
   // Objective-C
   #import <lrengine/core/LRRenderContext.h>
   #import <lrengine/factory/LRDeviceFactory.h>
   ```

## 💻 快速开始

### 创建Metal渲染上下文

```objc
#import <LREngine/LREngine.h>

// 创建渲染上下文
auto context = LR::LRDeviceFactory::CreateRenderContext(LR::BackendAPI::Metal);

// 初始化上下文（使用CAMetalLayer）
CAMetalLayer* metalLayer = /* 您的Metal Layer */;
context->Initialize((__bridge void*)metalLayer);

// 开始渲染
context->BeginFrame();
// ... 渲染命令 ...
context->EndFrame();
```

### 创建缓冲区

```cpp
// 创建顶点缓冲区
LR::BufferDescriptor bufferDesc;
bufferDesc.size = vertexDataSize;
bufferDesc.usage = LR::BufferUsage::Vertex;

auto vertexBuffer = context->CreateBuffer(bufferDesc, vertexData);
```

### 创建着色器

```cpp
// Metal着色器代码（MSL）
const char* vertexShaderCode = R"(
    #include <metal_stdlib>
    using namespace metal;
    
    vertex float4 vertexShader(uint vertexID [[vertex_id]]) {
        // 顶点着色器代码
    }
)";

LR::ShaderDescriptor shaderDesc;
shaderDesc.type = LR::ShaderType::Vertex;
shaderDesc.source = vertexShaderCode;

auto vertexShader = context->CreateShader(shaderDesc);
```

## 📋 系统要求

- **iOS版本**: iOS 13.0 及以上
- **架构**: arm64（iOS设备）
- **图形API**: Metal
- **语言**: C++17

## 🔍 API文档

### 核心类

- **LRRenderContext**: 渲染上下文，管理所有渲染操作
- **LRBuffer**: 缓冲区对象（顶点、索引、常量缓冲区等）
- **LRShader**: 着色器对象
- **LRTexture**: 纹理对象
- **LRPipelineState**: 渲染管线状态
- **LRFrameBuffer**: 帧缓冲区

### 工厂类

- **LRDeviceFactory**: 用于创建不同后端的渲染上下文

### 数学库

LREngine内置了轻量级数学库：

- `Vec2`, `Vec3`, `Vec4`: 向量类
- `Mat3`, `Mat4`: 矩阵类
- `Quaternion`: 四元数类

## ⚙️ 编译选项

本库在编译时启用了以下选项：

- `LRENGINE_ENABLE_METAL=ON`: 启用Metal后端
- `LRENGINE_PLATFORM_IOS`: iOS平台定义

## 🐛 调试

Debug版本包含以下额外功能：

- 详细的日志输出
- 运行时断言检查
- 调试符号

使用 `LRLog` 进行日志输出：

```cpp
#include <lrengine/utils/LRLog.h>

LR_LOG_INFO("初始化成功");
LR_LOG_ERROR("发生错误: %s", errorMessage);
```

## 📝 注意事项

1. **Metal仅限iOS设备**: 本构建仅支持iOS真机（arm64），不支持模拟器
2. **C++17要求**: 确保您的项目启用C++17标准
3. **ARC支持**: Framework内部使用ARC，无需手动管理Metal对象
4. **线程安全**: 渲染命令应在主线程或Metal线程中调用

## 📞 技术支持

如遇到问题，请检查：

1. 是否正确链接了所有必需的系统框架
2. 头文件搜索路径是否正确配置
3. 项目的最低部署目标是否为iOS 13.0或更高

## 📄 许可证

请参考LREngine项目的LICENSE文件。

---

**构建信息**:
- 构建日期: 由构建脚本自动生成
- 版本: 1.0.0
- 支持架构: arm64
- iOS最低版本: 13.0
EOF
    
    # 添加构建信息
    cat >> "$usage_file" << EOF

---

**本次构建信息**:
- 构建日期: $(date '+%Y-%m-%d %H:%M:%S')
- 构建配置: ${BUILD_CONFIG}
- 构建类型: ${BUILD_TYPE}
- iOS部署目标: ${IOS_DEPLOYMENT_TARGET}
- 支持架构: ${DEVICE_ARCHS}

EOF
    
    print_success "使用说明已生成: $usage_file"
}

################################################################################
# 主函数
################################################################################

main() {
    print_header "LREngine iOS 构建脚本"
    
    echo "项目路径: $PROJECT_ROOT"
    echo "输出目录: $OUTPUT_DIR"
    echo "构建配置: $BUILD_CONFIG"
    echo "构建类型: $BUILD_TYPE"
    echo "iOS版本: $IOS_DEPLOYMENT_TARGET"
    echo "支持架构: $DEVICE_ARCHS"
    echo ""
    
    # 检查环境
    check_environment
    
    # 创建输出目录
    mkdir -p "$OUTPUT_DIR"
    
    # 根据构建类型执行相应操作
    if [[ "$BUILD_TYPE" == "static" || "$BUILD_TYPE" == "all" ]]; then
        build_static_library
    fi
    
    if [[ "$BUILD_TYPE" == "framework" || "$BUILD_TYPE" == "all" ]]; then
        build_framework
    fi
    
    # 复制头文件（用于静态库）
    if [[ "$BUILD_TYPE" == "static" || "$BUILD_TYPE" == "all" ]]; then
        copy_headers
    fi
    
    # 生成使用说明
    generate_usage_guide
    
    # 完成
    print_header "构建完成！"
    
    echo "输出位置:"
    if [[ "$BUILD_TYPE" == "static" || "$BUILD_TYPE" == "all" ]]; then
        echo "  静态库: ${OUTPUT_DIR}/lib/liblrengine.a"
        echo "  头文件: ${OUTPUT_DIR}/include/lrengine/"
    fi
    if [[ "$BUILD_TYPE" == "framework" || "$BUILD_TYPE" == "all" ]]; then
        echo "  Framework: ${OUTPUT_DIR}/framework/${LIBRARY_NAME}.framework"
    fi
    echo "  使用说明: ${OUTPUT_DIR}/README.md"
    echo ""
    
    print_success "所有任务已完成！"
    print_info "请查看 ${OUTPUT_DIR}/README.md 了解如何在iOS项目中集成LREngine"
}

# 执行主函数
main
