#!/bin/bash

################################################################################
# LREngine Android 构建脚本
# 
# 功能：
# - 为Android平台交叉编译LREngine库
# - 生成共享库（.so）文件
# - 支持多种ABI架构（armeabi-v7a, arm64-v8a, x86, x86_64）
# - 支持Debug和Release配置
# - 自动配置OpenGL ES后端
#
# 使用方法：
#   ./build_android.sh [选项]
#
# 选项：
#   -c, --config <Debug|Release>  构建配置（默认：Release）
#   -a, --abi <架构>              目标架构（默认：all）
#   -o, --output <路径>           输出目录（默认：./build/android）
#   -l, --api-level <级别>        Android API级别（默认：21）
#   -s, --static                  生成静态库而非共享库
#   -h, --help                    显示帮助信息
#
# 示例：
#   ./build_android.sh                          # 构建所有架构的Release版本
#   ./build_android.sh -c Debug                 # 构建Debug版本
#   ./build_android.sh -a arm64-v8a             # 只构建arm64架构
#   ./build_android.sh -a "arm64-v8a armeabi-v7a"  # 构建指定多个架构
################################################################################

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# 默认参数
BUILD_CONFIG="Release"
TARGET_ABIS="all"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
OUTPUT_DIR="${PROJECT_ROOT}/build/android"
LIBRARY_NAME="lrengine"
VERSION="1.0.0"
BUILD_STATIC=false

# Android配置
ANDROID_SDK_ROOT="/Users/bigo/Library/Android/sdk"
ANDROID_API_LEVEL="21"

# 支持的所有ABI架构
ALL_ABIS="armeabi-v7a arm64-v8a x86 x86_64"

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

print_subheader() {
    echo ""
    echo -e "${CYAN}----------------------------------------${NC}"
    echo -e "${CYAN}$1${NC}"
    echo -e "${CYAN}----------------------------------------${NC}"
}

show_help() {
    cat << EOF
LREngine Android 构建脚本

用法: $0 [选项]

选项:
  -c, --config <Debug|Release>        构建配置（默认：Release）
  -a, --abi <架构>                    目标架构（默认：all）
  -o, --output <路径>                 输出目录（默认：./build/android）
  -l, --api-level <级别>              Android API级别（默认：21）
  -s, --static                        生成静态库（.a）而非共享库（.so）
  -h, --help                          显示此帮助信息

支持的ABI架构:
  armeabi-v7a   - 32位ARM（ARMv7）
  arm64-v8a     - 64位ARM（ARMv8/AArch64）
  x86           - 32位Intel x86
  x86_64        - 64位Intel x86_64
  all           - 构建所有架构

构建配置说明:
  Debug     - 包含调试符号，未优化，启用调试日志
  Release   - 优化构建，移除调试符号

示例:
  $0                                  # 构建所有架构的Release版本
  $0 -c Debug                         # 构建所有架构的Debug版本
  $0 -a arm64-v8a                     # 只构建64位ARM架构
  $0 -a "arm64-v8a armeabi-v7a"       # 构建64位和32位ARM架构
  $0 -l 24                            # 使用API Level 24
  $0 -s                               # 生成静态库
  $0 -o ~/output/android              # 输出到指定目录

输出文件结构:
  <输出目录>/
    ├── jniLibs/
    │   ├── armeabi-v7a/
    │   │   └── liblrengine.so
    │   ├── arm64-v8a/
    │   │   └── liblrengine.so
    │   ├── x86/
    │   │   └── liblrengine.so
    │   └── x86_64/
    │       └── liblrengine.so
    ├── include/
    │   └── lrengine/               # 公共头文件
    └── README.md                   # 使用说明

环境要求:
  - Android SDK（路径: ${ANDROID_SDK_ROOT}）
  - Android NDK（通过SDK安装）
  - CMake（3.15+）

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
        -a|--abi)
            TARGET_ABIS="$2"
            shift 2
            ;;
        -o|--output)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        -l|--api-level)
            ANDROID_API_LEVEL="$2"
            shift 2
            ;;
        -s|--static)
            BUILD_STATIC=true
            shift
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

# 处理目标架构
if [[ "$TARGET_ABIS" == "all" ]]; then
    TARGET_ABIS="$ALL_ABIS"
fi

# 验证架构参数
for abi in $TARGET_ABIS; do
    if [[ ! " $ALL_ABIS " =~ " $abi " ]]; then
        print_error "无效的ABI架构: $abi"
        print_info "支持的架构: $ALL_ABIS"
        exit 1
    fi
done

################################################################################
# 环境检查
################################################################################

check_environment() {
    print_header "检查构建环境"
    
    # 检查CMake
    if ! command -v cmake &> /dev/null; then
        print_error "未找到CMake，请先安装CMake"
        print_info "可以使用包管理器安装: brew install cmake (macOS) 或 apt install cmake (Linux)"
        exit 1
    fi
    CMAKE_VERSION=$(cmake --version | head -n1)
    print_success "CMake 版本: $CMAKE_VERSION"
    
    # 检查make或ninja
    if command -v ninja &> /dev/null; then
        BUILD_TOOL="Ninja"
        print_success "构建工具: Ninja"
    elif command -v make &> /dev/null; then
        BUILD_TOOL="Unix Makefiles"
        print_success "构建工具: Make"
    else
        print_error "未找到构建工具（Ninja或Make）"
        exit 1
    fi
    
    # 检查Android SDK
    if [[ ! -d "$ANDROID_SDK_ROOT" ]]; then
        print_error "未找到Android SDK: $ANDROID_SDK_ROOT"
        print_info "请确保Android SDK已安装在指定路径"
        print_info "或修改脚本中的 ANDROID_SDK_ROOT 变量"
        exit 1
    fi
    print_success "Android SDK: $ANDROID_SDK_ROOT"
    
    # 查找Android NDK
    # 优先查找ndk-bundle，其次查找ndk目录下最新版本
    if [[ -d "$ANDROID_SDK_ROOT/ndk-bundle" ]]; then
        ANDROID_NDK_ROOT="$ANDROID_SDK_ROOT/ndk-bundle"
    elif [[ -d "$ANDROID_SDK_ROOT/ndk" ]]; then
        # 查找最新版本的NDK
        ANDROID_NDK_ROOT=$(ls -d "$ANDROID_SDK_ROOT/ndk"/*/ 2>/dev/null | sort -V | tail -n1 | sed 's:/$::')
    fi
    
    if [[ -z "$ANDROID_NDK_ROOT" || ! -d "$ANDROID_NDK_ROOT" ]]; then
        print_error "未找到Android NDK"
        print_info "请通过Android SDK Manager安装NDK:"
        print_info "  sdkmanager --install 'ndk;25.2.9519653'"
        exit 1
    fi
    print_success "Android NDK: $ANDROID_NDK_ROOT"
    
    # 获取NDK版本
    if [[ -f "$ANDROID_NDK_ROOT/source.properties" ]]; then
        NDK_VERSION=$(grep "Pkg.Revision" "$ANDROID_NDK_ROOT/source.properties" | cut -d'=' -f2 | tr -d ' ')
        print_info "NDK 版本: $NDK_VERSION"
    fi
    
    # 查找CMake工具链文件
    CMAKE_TOOLCHAIN="$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake"
    if [[ ! -f "$CMAKE_TOOLCHAIN" ]]; then
        print_error "未找到Android CMake工具链文件"
        print_info "预期路径: $CMAKE_TOOLCHAIN"
        exit 1
    fi
    print_success "CMake工具链: $CMAKE_TOOLCHAIN"
    
    # 检查平台特定工具
    if [[ -d "$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt" ]]; then
        print_success "NDK LLVM工具链已安装"
    else
        print_warning "未找到LLVM工具链，可能影响编译"
    fi
    
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
# 构建单个架构
################################################################################

build_for_abi() {
    local abi=$1
    
    print_subheader "构建 $abi 架构 (${BUILD_CONFIG})"
    
    local build_dir="${OUTPUT_DIR}/build_${abi}_${BUILD_CONFIG}"
    clean_build_dir "$build_dir"
    mkdir -p "$build_dir"
    
    # 确定库类型
    if [[ "$BUILD_STATIC" == true ]]; then
        LIB_TYPE="STATIC"
        LIB_EXTENSION="a"
        LIB_PREFIX="lib"
    else
        LIB_TYPE="SHARED"
        LIB_EXTENSION="so"
        LIB_PREFIX="lib"
    fi
    
    print_info "配置CMake..."
    
    # 设置CMake参数
    local cmake_args=(
        -S "$PROJECT_ROOT"
        -B "$build_dir"
        -G "$BUILD_TOOL"
        -DCMAKE_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN"
        -DANDROID_ABI="$abi"
        -DANDROID_PLATFORM="android-${ANDROID_API_LEVEL}"
        -DANDROID_NDK="$ANDROID_NDK_ROOT"
        -DANDROID_STL=c++_shared
        -DCMAKE_BUILD_TYPE="$BUILD_CONFIG"
        -DCMAKE_ANDROID_ARCH_ABI="$abi"
        -DLRENGINE_ENABLE_OPENGLES=ON
        -DLRENGINE_ENABLE_OPENGL=OFF
        -DLRENGINE_ENABLE_METAL=OFF
        -DLRENGINE_ENABLE_VULKAN=OFF
        -DLRENGINE_BUILD_EXAMPLES=OFF
        -DLRENGINE_BUILD_TESTS=OFF
        -DBUILD_SHARED_LIBS=$([[ "$BUILD_STATIC" == true ]] && echo "OFF" || echo "ON")
    )
    
    # 针对不同架构的优化
    case "$abi" in
        armeabi-v7a)
            cmake_args+=(-DANDROID_ARM_NEON=ON)
            ;;
        arm64-v8a)
            # arm64默认启用NEON
            ;;
    esac
    
    # 执行CMake配置
    cmake "${cmake_args[@]}"
    
    if [[ $? -ne 0 ]]; then
        print_error "CMake配置失败: $abi"
        return 1
    fi
    
    print_info "编译库文件..."
    cmake --build "$build_dir" --config "$BUILD_CONFIG" --target lrengine -- -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    if [[ $? -ne 0 ]]; then
        print_error "编译失败: $abi"
        return 1
    fi
    
    # 复制库文件到输出目录
    local output_lib_dir="${OUTPUT_DIR}/jniLibs/${abi}"
    mkdir -p "$output_lib_dir"
    
    # 查找编译的库文件
    local lib_file=""
    if [[ "$BUILD_STATIC" == true ]]; then
        lib_file=$(find "$build_dir" -name "${LIB_PREFIX}${LIBRARY_NAME}.${LIB_EXTENSION}" -type f | head -n1)
    else
        lib_file=$(find "$build_dir" -name "${LIB_PREFIX}${LIBRARY_NAME}.${LIB_EXTENSION}" -type f | head -n1)
    fi
    
    if [[ -z "$lib_file" || ! -f "$lib_file" ]]; then
        print_error "未找到编译的库文件: $abi"
        print_info "搜索路径: $build_dir"
        return 1
    fi
    
    cp "$lib_file" "$output_lib_dir/"
    
    # 如果是共享库，还需要复制STL库
    if [[ "$BUILD_STATIC" == false ]]; then
        local stl_lib="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/*/sysroot/usr/lib/${abi}/libc++_shared.so"
        stl_lib=$(ls $stl_lib 2>/dev/null | head -n1)
        if [[ -f "$stl_lib" ]]; then
            cp "$stl_lib" "$output_lib_dir/"
            print_info "已复制STL库: libc++_shared.so"
        fi
    fi
    
    # 获取库信息
    local lib_size=$(du -h "$output_lib_dir/${LIB_PREFIX}${LIBRARY_NAME}.${LIB_EXTENSION}" | awk '{print $1}')
    
    print_success "$abi 构建完成"
    print_info "  库文件: $output_lib_dir/${LIB_PREFIX}${LIBRARY_NAME}.${LIB_EXTENSION}"
    print_info "  大小: $lib_size"
    
    # 显示库依赖（如果可用）
    if command -v readelf &> /dev/null && [[ "$BUILD_STATIC" == false ]]; then
        print_info "  依赖库:"
        readelf -d "$output_lib_dir/${LIB_PREFIX}${LIBRARY_NAME}.${LIB_EXTENSION}" 2>/dev/null | grep NEEDED | awk '{print "    " $5}' | tr -d '[]'
    fi
    
    echo ""
    return 0
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
# LREngine Android 库使用指南

LREngine是一个轻量级跨平台渲染引擎，本文档说明如何在Android项目中集成和使用LREngine。

## 📦 包含内容

构建完成后，您将获得以下文件：

```
android/
├── jniLibs/
│   ├── armeabi-v7a/
│   │   ├── liblrengine.so
│   │   └── libc++_shared.so
│   ├── arm64-v8a/
│   │   ├── liblrengine.so
│   │   └── libc++_shared.so
│   ├── x86/
│   │   ├── liblrengine.so
│   │   └── libc++_shared.so
│   └── x86_64/
│       ├── liblrengine.so
│       └── libc++_shared.so
├── include/
│   └── lrengine/               # 公共头文件
└── README.md                   # 本文档
```

## 🔧 集成方式

### 方式一：Android Studio项目集成

1. **复制库文件**
   
   将 `jniLibs` 目录复制到您的Android项目中：
   ```
   app/src/main/jniLibs/
   ```

2. **配置build.gradle**
   
   在 `app/build.gradle` 中添加：
   ```groovy
   android {
       // ...
       
       defaultConfig {
           // 指定支持的ABI
           ndk {
               abiFilters 'armeabi-v7a', 'arm64-v8a', 'x86', 'x86_64'
           }
         }
       
       // JNI库目录
       sourceSets {
           main {
               jniLibs.srcDirs = ['src/main/jniLibs']
           }
       }
   }
   ```

3. **添加JNI桥接代码**
   
   创建JNI桥接层来调用C++代码。

### 方式二：CMake集成（推荐用于NDK开发）

1. **项目结构**
   ```
   app/
   ├── src/main/
   │   ├── cpp/
   │   │   ├── CMakeLists.txt
   │   │   └── native-lib.cpp
   │   └── jniLibs/
   │       └── [复制的库文件]
   ```

2. **CMakeLists.txt示例**
   ```cmake
   cmake_minimum_required(VERSION 3.18.1)
   project(myapp)
   
   # 设置C++标准
   set(CMAKE_CXX_STANDARD 17)
   
   # 添加LREngine预编译库
   add_library(lrengine SHARED IMPORTED)
   set_target_properties(lrengine PROPERTIES
       IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}/liblrengine.so
   )
   
   # 头文件路径
   set(LRENGINE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
   
   # 您的原生库
   add_library(native-lib SHARED native-lib.cpp)
   
   target_include_directories(native-lib PRIVATE ${LRENGINE_INCLUDE_DIR})
   target_link_libraries(native-lib
       lrengine
       GLESv3
       EGL
       android
       log
   )
   ```

3. **build.gradle配置**
   ```groovy
   android {
       // ...
       
       externalNativeBuild {
           cmake {
               path "src/main/cpp/CMakeLists.txt"
               version "3.18.1"
           }
       }
       
       defaultConfig {
           externalNativeBuild {
               cmake {
                   cppFlags "-std=c++17"
                   arguments "-DANDROID_STL=c++_shared"
               }
           }
       }
   }
   ```

## 💻 快速开始

### JNI桥接层示例

**native-lib.cpp:**
```cpp
#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <lrengine/factory/LRDeviceFactory.h>
#include <lrengine/core/LRRenderContext.h>

using namespace lrengine::render;

// 全局渲染上下文
static LRRenderContext* g_context = nullptr;

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_myapp_LREngineRenderer_nativeInit(
        JNIEnv* env,
        jobject /* this */,
        jobject surface) {
    
    // 获取ANativeWindow
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (!window) {
        return JNI_FALSE;
    }
    
    // 创建OpenGL ES渲染上下文
    g_context = LRDeviceFactory::CreateRenderContext(Backend::OpenGLES);
    if (!g_context) {
        ANativeWindow_release(window);
        return JNI_FALSE;
    }
    
    // 初始化上下文
    if (!g_context->Initialize(window)) {
        delete g_context;
        g_context = nullptr;
        ANativeWindow_release(window);
        return JNI_FALSE;
    }
    
    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_myapp_LREngineRenderer_nativeRender(
        JNIEnv* env,
        jobject /* this */) {
    
    if (!g_context) return;
    
    // 开始帧
    g_context->BeginFrame();
    
    // 清除屏幕
    float clearColor[] = {0.2f, 0.3f, 0.4f, 1.0f};
    g_context->Clear(ClearColor | ClearDepth, clearColor, 1.0f, 0);
    
    // ... 渲染代码 ...
    
    // 结束帧
    g_context->EndFrame();
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_myapp_LREngineRenderer_nativeDestroy(
        JNIEnv* env,
        jobject /* this */) {
    
    if (g_context) {
        g_context->Shutdown();
        delete g_context;
        g_context = nullptr;
    }
}
```

### Java/Kotlin层示例

**LREngineRenderer.kt:**
```kotlin
class LREngineRenderer : GLSurfaceView.Renderer {
    
    companion object {
        init {
            System.loadLibrary("c++_shared")
            System.loadLibrary("lrengine")
            System.loadLibrary("native-lib")
        }
    }
    
    external fun nativeInit(surface: Surface): Boolean
    external fun nativeRender()
    external fun nativeDestroy()
    
    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
        // Surface创建回调
    }
    
    override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
        // 尺寸变化回调
    }
    
    override fun onDrawFrame(gl: GL10?) {
        nativeRender()
    }
}
```

## 📋 系统要求

- **Android版本**: API Level 21+ (Android 5.0 Lollipop)
- **支持架构**: armeabi-v7a, arm64-v8a, x86, x86_64
- **图形API**: OpenGL ES 3.0+
- **C++标准**: C++17
- **STL**: libc++_shared

## 🔍 API概览

### 核心类

- **LRRenderContext**: 渲染上下文，管理所有渲染操作
- **LRBuffer**: 缓冲区对象（顶点、索引、Uniform缓冲区等）
- **LRShader**: 着色器对象（支持GLSL ES）
- **LRTexture**: 纹理对象
- **LRPipelineState**: 渲染管线状态
- **LRFrameBuffer**: 帧缓冲区（用于离屏渲染）

### 工厂类

- **LRDeviceFactory**: 用于创建不同后端的渲染上下文

### 数学库

LREngine内置了轻量级数学库：

- `Vec2`, `Vec3`, `Vec4`: 向量类
- `Mat3`, `Mat4`: 矩阵类
- `Quaternion`: 四元数类

## ⚙️ OpenGL ES着色器

LREngine在Android上使用OpenGL ES 3.0+，着色器需要使用GLSL ES语法：

```glsl
#version 300 es
precision highp float;

// 顶点着色器
in vec3 aPosition;
in vec2 aTexCoord;

out vec2 vTexCoord;

uniform mat4 uMVP;

void main() {
    gl_Position = uMVP * vec4(aPosition, 1.0);
    vTexCoord = aTexCoord;
}
```

```glsl
#version 300 es
precision highp float;

// 片段着色器
in vec2 vTexCoord;
out vec4 fragColor;

uniform sampler2D uTexture;

void main() {
    fragColor = texture(uTexture, vTexCoord);
}
```

## 🐛 调试

### 日志输出

Debug版本会输出详细日志到Android Logcat：

```cpp
#include <lrengine/utils/LRLog.h>

LR_LOG_INFO("初始化成功");
LR_LOG_ERROR_F("发生错误: %s", errorMessage);
```

在Logcat中过滤标签 `LREngine` 查看日志。

### 常见问题

1. **UnsatisfiedLinkError**
   - 确保所有必需的.so文件都已复制
   - 检查库加载顺序（先加载c++_shared，再加载lrengine）
   - 确认ABI架构匹配

2. **EGL错误**
   - 确保在正确的线程中调用OpenGL ES
   - 检查Surface是否有效

3. **渲染黑屏**
   - 检查着色器编译日志
   - 确认顶点数据格式正确
   - 检查Clear调用是否正确

## 📝 注意事项

1. **线程安全**: OpenGL ES调用必须在创建上下文的线程中进行
2. **生命周期**: 在Activity/Fragment销毁时调用nativeDestroy释放资源
3. **STL库**: 使用c++_shared需要确保所有原生库使用相同的STL
4. **ProGuard**: 如使用ProGuard，确保JNI方法名不被混淆

## 📄 许可证

请参考LREngine项目的LICENSE文件。

EOF
    
    # 添加构建信息
    cat >> "$usage_file" << EOF

---

**本次构建信息**:
- 构建日期: $(date '+%Y-%m-%d %H:%M:%S')
- 构建配置: ${BUILD_CONFIG}
- 目标架构: ${TARGET_ABIS}
- Android API Level: ${ANDROID_API_LEVEL}
- 库类型: $([[ "$BUILD_STATIC" == true ]] && echo "静态库(.a)" || echo "共享库(.so)")
- NDK版本: ${NDK_VERSION:-未知}

EOF
    
    print_success "使用说明已生成: $usage_file"
}

################################################################################
# 构建摘要
################################################################################

print_build_summary() {
    print_header "构建摘要"
    
    echo "输出目录: $OUTPUT_DIR"
    echo ""
    
    local total_size=0
    local built_count=0
    
    for abi in $TARGET_ABIS; do
        local lib_file="${OUTPUT_DIR}/jniLibs/${abi}/${LIB_PREFIX}${LIBRARY_NAME}.${LIB_EXTENSION}"
        if [[ -f "$lib_file" ]]; then
            local size=$(du -h "$lib_file" | awk '{print $1}')
            echo "  $abi: $size"
            ((built_count++))
        else
            echo "  $abi: 未构建"
        fi
    done
    
    echo ""
    echo "构建完成: $built_count/${#TARGET_ABIS[@]} 个架构"
    
    if [[ -d "${OUTPUT_DIR}/jniLibs" ]]; then
        local total=$(du -sh "${OUTPUT_DIR}/jniLibs" | awk '{print $1}')
        echo "总大小: $total"
    fi
}

################################################################################
# 主函数
################################################################################

main() {
    print_header "LREngine Android 构建脚本"
    
    echo "项目路径: $PROJECT_ROOT"
    echo "输出目录: $OUTPUT_DIR"
    echo "构建配置: $BUILD_CONFIG"
    echo "目标架构: $TARGET_ABIS"
    echo "API Level: $ANDROID_API_LEVEL"
    echo "库类型: $([[ "$BUILD_STATIC" == true ]] && echo "静态库" || echo "共享库")"
    echo ""
    
    # 检查环境
    check_environment
    
    # 创建输出目录
    mkdir -p "$OUTPUT_DIR"
    
    # 设置库类型变量
    if [[ "$BUILD_STATIC" == true ]]; then
        LIB_EXTENSION="a"
        LIB_PREFIX="lib"
    else
        LIB_EXTENSION="so"
        LIB_PREFIX="lib"
    fi
    
    # 构建各架构
    local failed_abis=""
    for abi in $TARGET_ABIS; do
        if ! build_for_abi "$abi"; then
            failed_abis+="$abi "
        fi
    done
    
    # 检查是否有失败
    if [[ -n "$failed_abis" ]]; then
        print_warning "以下架构构建失败: $failed_abis"
    fi
    
    # 复制头文件
    copy_headers
    
    # 生成使用说明
    generate_usage_guide
    
    # 打印构建摘要
    print_build_summary
    
    # 完成
    print_header "构建完成！"
    
    echo "输出位置:"
    echo "  库文件: ${OUTPUT_DIR}/jniLibs/"
    echo "  头文件: ${OUTPUT_DIR}/include/lrengine/"
    echo "  使用说明: ${OUTPUT_DIR}/README.md"
    echo ""
    
    if [[ -z "$failed_abis" ]]; then
        print_success "所有任务已完成！"
    else
        print_warning "部分架构构建失败，请检查错误信息"
    fi
    
    print_info "请查看 ${OUTPUT_DIR}/README.md 了解如何在Android项目中集成LREngine"
    
    # 拷贝库文件到Android Demo项目
    copy_to_android_demo
}

################################################################################
# 拷贝库文件到Android Demo项目
################################################################################

copy_to_android_demo() {
    local demo_jniLibs_dir="${PROJECT_ROOT}/demo/LREngineAndroid/app/src/main/jniLibs"
    local demo_include_dir="${PROJECT_ROOT}/demo/LREngineAndroid/app/src/main/cpp/include/lrengine"
    local source_jniLibs_dir="${OUTPUT_DIR}/jniLibs"
    local source_include_dir="${PROJECT_ROOT}/include/lrengine"
    
    print_subheader "拷贝文件到Android Demo项目"
    
    # ========== 拷贝库文件 ==========
    if [[ -d "$source_jniLibs_dir" ]]; then
        # 创建目标目录（如果不存在）
        mkdir -p "$demo_jniLibs_dir"
        
        # 拷贝各架构的库文件
        local copied_count=0
        for abi in $TARGET_ABIS; do
            local src_abi_dir="${source_jniLibs_dir}/${abi}"
            local dst_abi_dir="${demo_jniLibs_dir}/${abi}"
            
            if [[ -d "$src_abi_dir" ]]; then
                # 创建目标架构目录
                mkdir -p "$dst_abi_dir"
                
                # 拷贝所有库文件（.so 和 .a）
                cp -f "${src_abi_dir}"/*.so "$dst_abi_dir/" 2>/dev/null && {
                    print_info "已拷贝 $abi 架构库文件到 Demo 项目"
                    ((copied_count++))
                } || true
                
                cp -f "${src_abi_dir}"/*.a "$dst_abi_dir/" 2>/dev/null || true
            fi
        done
        
        if [[ $copied_count -gt 0 ]]; then
            print_success "已拷贝 $copied_count 个架构的库文件到: $demo_jniLibs_dir"
        else
            print_warning "没有库文件被拷贝到Demo项目"
        fi
    else
        print_warning "源jniLibs目录不存在，跳过拷贝库文件"
    fi
    
    # ========== 拷贝头文件 ==========
    if [[ -d "$source_include_dir" ]]; then
        print_info "同步头文件到 Demo 项目..."
        
        # 删除旧的头文件目录并重新创建
        rm -rf "$demo_include_dir"
        mkdir -p "$demo_include_dir"
        
        # 拷贝所有头文件（保持目录结构）
        cp -R "${source_include_dir}/"* "$demo_include_dir/"
        
        # 统计拷贝的文件数量
        local header_count=$(find "$demo_include_dir" -type f \( -name "*.h" -o -name "*.hpp" \) | wc -l | tr -d ' ')
        print_success "已同步 $header_count 个头文件到: $demo_include_dir"
    else
        print_warning "源include目录不存在，跳过拷贝头文件"
    fi
    
    # ========== 显示拷贝结果 ==========
    echo ""
    print_info "Demo项目文件结构:"
    if command -v tree &> /dev/null; then
        echo "jniLibs:"
        tree -L 2 "$demo_jniLibs_dir" 2>/dev/null || ls -la "$demo_jniLibs_dir" 2>/dev/null || echo "  (目录不存在)"
        echo ""
        echo "include:"
        tree -L 2 "${PROJECT_ROOT}/demo/LREngineAndroid/app/src/main/cpp/include" 2>/dev/null || ls -la "${PROJECT_ROOT}/demo/LREngineAndroid/app/src/main/cpp/include" 2>/dev/null || echo "  (目录不存在)"
    else
        ls -laR "$demo_jniLibs_dir" 2>/dev/null || echo "jniLibs目录不存在"
    fi
}

# 执行主函数
main
