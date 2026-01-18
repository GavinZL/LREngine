# LREngine Android 集成指南

本指南详细说明如何使用 `build_android.sh` 脚本构建 LREngine Android 库，以及如何在 Android 项目中集成使用。

## 目录

- [环境准备](#环境准备)
- [构建脚本使用](#构建脚本使用)
- [项目集成](#项目集成)
- [JNI开发示例](#jni开发示例)
- [常见问题](#常见问题)

## 环境准备

### 1. 安装 Android SDK

确保 Android SDK 已安装在以下路径：
```
/Users/bigo/Library/Android/sdk
```

如果使用不同路径，需要修改 `build_android.sh` 中的 `ANDROID_SDK_ROOT` 变量。

### 2. 安装 Android NDK

通过 Android SDK Manager 安装 NDK：

```bash
# 使用命令行
$ANDROID_SDK_ROOT/cmdline-tools/latest/bin/sdkmanager --install "ndk;25.2.9519653"

# 或通过 Android Studio
# Settings -> SDK Manager -> SDK Tools -> NDK (Side by side)
```

推荐 NDK 版本：**25.x** 或更高版本。

### 3. 安装 CMake

```bash
# macOS
brew install cmake

# Linux
sudo apt-get install cmake

# 或通过 SDK Manager
sdkmanager --install "cmake;3.22.1"
```

### 4. 验证环境

```bash
# 检查 CMake
cmake --version

# 检查 NDK
ls $ANDROID_SDK_ROOT/ndk/
```

## 构建脚本使用

### 基本用法

```bash
cd script
./build_android.sh
```

这将构建所有架构（armeabi-v7a, arm64-v8a, x86, x86_64）的 Release 版本共享库。

### 命令行选项

| 选项 | 说明 | 默认值 |
|------|------|--------|
| `-c, --config` | 构建配置 (Debug/Release) | Release |
| `-a, --abi` | 目标架构 | all |
| `-o, --output` | 输出目录 | ./build/android |
| `-l, --api-level` | Android API 级别 | 21 |
| `-s, --static` | 生成静态库 | 否（共享库） |
| `-h, --help` | 显示帮助 | - |

### 使用示例

```bash
# 构建 Debug 版本
./build_android.sh -c Debug

# 只构建 64位 ARM 架构
./build_android.sh -a arm64-v8a

# 构建 ARM 架构（32位和64位）
./build_android.sh -a "armeabi-v7a arm64-v8a"

# 使用 API Level 24
./build_android.sh -l 24

# 生成静态库
./build_android.sh -s

# 输出到自定义目录
./build_android.sh -o ~/Desktop/LREngine-Android

# 组合选项
./build_android.sh -c Debug -a arm64-v8a -l 24
```

### 输出结构

构建完成后，输出目录结构如下：

```
build/android/
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
│   └── lrengine/
│       ├── core/
│       ├── factory/
│       ├── math/
│       └── utils/
└── README.md
```

## 项目集成

### 方式一：直接复制 jniLibs

最简单的集成方式，适合快速原型开发。

1. 复制库文件到 Android 项目：
   ```bash
   cp -r build/android/jniLibs your_project/app/src/main/
   ```

2. 在 `app/build.gradle` 中配置：
   ```groovy
   android {
       defaultConfig {
           ndk {
               abiFilters 'armeabi-v7a', 'arm64-v8a', 'x86', 'x86_64'
           }
       }
       
       sourceSets {
           main {
               jniLibs.srcDirs = ['src/main/jniLibs']
           }
       }
   }
   ```

### 方式二：CMake 集成（推荐）

适合需要编写 JNI 代码的项目。

1. 项目结构：
   ```
   app/
   ├── src/main/
   │   ├── cpp/
   │   │   ├── CMakeLists.txt
   │   │   ├── include/
   │   │   │   └── lrengine/  (复制的头文件)
   │   │   └── native-lib.cpp
   │   └── jniLibs/  (复制的库文件)
   ```

2. `CMakeLists.txt`：
   ```cmake
   cmake_minimum_required(VERSION 3.18.1)
   project(myapp)
   
   set(CMAKE_CXX_STANDARD 17)
   
   # LREngine 预编译库
   add_library(lrengine SHARED IMPORTED)
   set_target_properties(lrengine PROPERTIES
       IMPORTED_LOCATION 
       ${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}/liblrengine.so
   )
   
   # 头文件
   include_directories(${CMAKE_SOURCE_DIR}/include)
   
   # 原生库
   add_library(native-lib SHARED native-lib.cpp)
   
   target_link_libraries(native-lib
       lrengine
       GLESv3
       EGL
       android
       log
   )
   ```

3. `app/build.gradle`：
   ```groovy
   android {
       defaultConfig {
           externalNativeBuild {
               cmake {
                   cppFlags "-std=c++17"
                   arguments "-DANDROID_STL=c++_shared"
               }
           }
           ndk {
               abiFilters 'armeabi-v7a', 'arm64-v8a'
           }
       }
       
       externalNativeBuild {
           cmake {
               path "src/main/cpp/CMakeLists.txt"
               version "3.18.1"
           }
       }
   }
   ```

### 方式三：AAR 打包（企业级）

将库打包为 AAR 格式，便于分发。

```bash
# 创建 AAR 项目结构后，使用 Gradle 打包
./gradlew assembleRelease
```

## JNI 开发示例

### 1. JNI 桥接头文件 (native-lib.h)

```cpp
#ifndef NATIVE_LIB_H
#define NATIVE_LIB_H

#include <jni.h>

extern "C" {
    JNIEXPORT jboolean JNICALL
    Java_com_example_app_LREngineJNI_nativeInit(
        JNIEnv* env, jobject thiz, jobject surface);
    
    JNIEXPORT void JNICALL
    Java_com_example_app_LREngineJNI_nativeResize(
        JNIEnv* env, jobject thiz, jint width, jint height);
    
    JNIEXPORT void JNICALL
    Java_com_example_app_LREngineJNI_nativeRender(
        JNIEnv* env, jobject thiz);
    
    JNIEXPORT void JNICALL
    Java_com_example_app_LREngineJNI_nativeDestroy(
        JNIEnv* env, jobject thiz);
}

#endif
```

### 2. JNI 实现 (native-lib.cpp)

```cpp
#include "native-lib.h"

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>

#include <lrengine/factory/LRDeviceFactory.h>
#include <lrengine/core/LRRenderContext.h>
#include <lrengine/core/LRBuffer.h>
#include <lrengine/core/LRShader.h>

#define LOG_TAG "LREngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace lrengine::render;

// 全局变量
static LRRenderContext* g_context = nullptr;
static ANativeWindow* g_window = nullptr;
static int g_width = 0;
static int g_height = 0;

JNIEXPORT jboolean JNICALL
Java_com_example_app_LREngineJNI_nativeInit(
        JNIEnv* env, jobject thiz, jobject surface) {
    
    LOGI("Initializing LREngine...");
    
    // 获取原生窗口
    g_window = ANativeWindow_fromSurface(env, surface);
    if (!g_window) {
        LOGE("Failed to get ANativeWindow");
        return JNI_FALSE;
    }
    
    // 创建 OpenGL ES 上下文
    g_context = LRDeviceFactory::CreateRenderContext(Backend::OpenGLES);
    if (!g_context) {
        LOGE("Failed to create render context");
        ANativeWindow_release(g_window);
        g_window = nullptr;
        return JNI_FALSE;
    }
    
    // 初始化
    if (!g_context->Initialize(g_window)) {
        LOGE("Failed to initialize render context");
        delete g_context;
        g_context = nullptr;
        ANativeWindow_release(g_window);
        g_window = nullptr;
        return JNI_FALSE;
    }
    
    LOGI("LREngine initialized successfully");
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_example_app_LREngineJNI_nativeResize(
        JNIEnv* env, jobject thiz, jint width, jint height) {
    
    g_width = width;
    g_height = height;
    
    if (g_context) {
        g_context->SetViewport(0, 0, width, height);
    }
    
    LOGI("Resize: %dx%d", width, height);
}

JNIEXPORT void JNICALL
Java_com_example_app_LREngineJNI_nativeRender(
        JNIEnv* env, jobject thiz) {
    
    if (!g_context) return;
    
    // 开始帧
    g_context->BeginFrame();
    
    // 清除屏幕
    float clearColor[] = {0.2f, 0.3f, 0.4f, 1.0f};
    g_context->Clear(ClearColor | ClearDepth, clearColor, 1.0f, 0);
    
    // TODO: 在这里添加渲染代码
    
    // 结束帧
    g_context->EndFrame();
}

JNIEXPORT void JNICALL
Java_com_example_app_LREngineJNI_nativeDestroy(
        JNIEnv* env, jobject thiz) {
    
    LOGI("Destroying LREngine...");
    
    if (g_context) {
        g_context->Shutdown();
        delete g_context;
        g_context = nullptr;
    }
    
    if (g_window) {
        ANativeWindow_release(g_window);
        g_window = nullptr;
    }
    
    LOGI("LREngine destroyed");
}
```

### 3. Java/Kotlin 接口

**LREngineJNI.kt:**
```kotlin
package com.example.app

import android.view.Surface

object LREngineJNI {
    
    init {
        System.loadLibrary("c++_shared")
        System.loadLibrary("lrengine")
        System.loadLibrary("native-lib")
    }
    
    external fun nativeInit(surface: Surface): Boolean
    external fun nativeResize(width: Int, height: Int)
    external fun nativeRender()
    external fun nativeDestroy()
}
```

**LREngineSurfaceView.kt:**
```kotlin
package com.example.app

import android.content.Context
import android.view.SurfaceHolder
import android.view.SurfaceView

class LREngineSurfaceView(context: Context) : SurfaceView(context), 
    SurfaceHolder.Callback {
    
    private var renderThread: RenderThread? = null
    
    init {
        holder.addCallback(this)
    }
    
    override fun surfaceCreated(holder: SurfaceHolder) {
        if (LREngineJNI.nativeInit(holder.surface)) {
            renderThread = RenderThread().also { it.start() }
        }
    }
    
    override fun surfaceChanged(holder: SurfaceHolder, format: Int, 
                                 width: Int, height: Int) {
        LREngineJNI.nativeResize(width, height)
    }
    
    override fun surfaceDestroyed(holder: SurfaceHolder) {
        renderThread?.requestStop()
        renderThread = null
        LREngineJNI.nativeDestroy()
    }
    
    private inner class RenderThread : Thread() {
        @Volatile
        private var running = true
        
        fun requestStop() {
            running = false
            try {
                join()
            } catch (e: InterruptedException) {
                e.printStackTrace()
            }
        }
        
        override fun run() {
            while (running) {
                LREngineJNI.nativeRender()
                // 控制帧率
                Thread.sleep(16) // ~60 FPS
            }
        }
    }
}
```

## 常见问题

### Q1: UnsatisfiedLinkError 异常

**原因**: 库文件未正确加载或架构不匹配。

**解决方案**:
1. 确保所有 `.so` 文件都已复制到 `jniLibs` 目录
2. 检查库加载顺序（先 c++_shared，再 lrengine，最后自己的库）
3. 确认设备架构与库架构匹配

### Q2: 找不到 EGL/OpenGL ES 错误

**原因**: 缺少系统库链接。

**解决方案**:
在 CMakeLists.txt 中添加：
```cmake
target_link_libraries(native-lib GLESv3 EGL android log)
```

### Q3: 构建失败 "NDK not found"

**原因**: NDK 未安装或路径错误。

**解决方案**:
1. 通过 SDK Manager 安装 NDK
2. 检查 `build_android.sh` 中的 `ANDROID_SDK_ROOT` 路径
3. 手动设置 `ANDROID_NDK_ROOT` 环境变量

### Q4: 库太大

**解决方案**:
1. 使用 Release 构建：`./build_android.sh -c Release`
2. 只构建需要的架构：`./build_android.sh -a arm64-v8a`
3. 启用 ProGuard/R8 进行代码压缩

### Q5: 渲染黑屏

**排查步骤**:
1. 检查 Logcat 中的错误日志
2. 确认 Surface 有效
3. 检查着色器编译日志
4. 验证 Clear 颜色是否正确设置

## 性能优化建议

1. **只构建需要的架构**
   - 大多数现代设备是 arm64-v8a
   - 模拟器通常是 x86 或 x86_64

2. **使用 Release 构建**
   - Release 版本启用优化，体积更小

3. **减少 JNI 调用**
   - 批量处理渲染命令
   - 避免频繁的 Java/Native 切换

4. **合理使用多线程**
   - 渲染操作在单独线程
   - 资源加载使用后台线程

## 参考链接

- [Android NDK 官方文档](https://developer.android.com/ndk)
- [CMake Android 工具链](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html#cross-compiling-for-android)
- [JNI 开发指南](https://developer.android.com/training/articles/perf-jni)
