/**
 * @file LRDefines.h
 * @brief LREngine宏定义和导出符号
 */

#pragma once

#include <cstdint>
#include <cstddef>

// 版本定义
#define LRENGINE_VERSION_MAJOR 1
#define LRENGINE_VERSION_MINOR 0
#define LRENGINE_VERSION_PATCH 0

// 平台检测
#if defined(_WIN32) || defined(_WIN64)
    #define LR_PLATFORM_WINDOWS 1
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        #define LR_PLATFORM_IOS 1
    #else
        #define LR_PLATFORM_MACOS 1
    #endif
    #define LR_PLATFORM_APPLE 1
#elif defined(__ANDROID__)
    #define LR_PLATFORM_ANDROID 1
#elif defined(__linux__)
    #define LR_PLATFORM_LINUX 1
#else
    #error "Unsupported platform"
#endif

// 编译器检测
#if defined(_MSC_VER)
    #define LR_COMPILER_MSVC 1
#elif defined(__clang__)
    #define LR_COMPILER_CLANG 1
#elif defined(__GNUC__)
    #define LR_COMPILER_GCC 1
#else
    #define LR_COMPILER_UNKNOWN 1
#endif

// API导出宏
#if defined(LR_PLATFORM_WINDOWS)
    #if defined(LRENGINE_EXPORT)
        #define LR_API __declspec(dllexport)
    #elif defined(LRENGINE_IMPORT)
        #define LR_API __declspec(dllimport)
    #else
        #define LR_API
    #endif
#else
    #if defined(LRENGINE_EXPORT)
        #define LR_API __attribute__((visibility("default")))
    #else
        #define LR_API
    #endif
#endif

// 调用约定
#if defined(LR_PLATFORM_WINDOWS)
    #define LR_CALL __stdcall
#else
    #define LR_CALL
#endif

// 内联提示
#if defined(LR_COMPILER_MSVC)
    #define LR_FORCEINLINE __forceinline
#elif defined(LR_COMPILER_GCC) || defined(LR_COMPILER_CLANG)
    #define LR_FORCEINLINE __attribute__((always_inline)) inline
#else
    #define LR_FORCEINLINE inline
#endif

// 禁用拷贝宏
#define LR_NONCOPYABLE(ClassName) \
    ClassName(const ClassName&) = delete; \
    ClassName& operator=(const ClassName&) = delete

// 禁用移动宏
#define LR_NONMOVABLE(ClassName) \
    ClassName(ClassName&&) = delete; \
    ClassName& operator=(ClassName&&) = delete

// 断言宏
#if defined(LR_DEBUG) || defined(_DEBUG)
    #include <cassert>
    #define LR_ASSERT(condition) assert(condition)
    #define LR_ASSERT_MSG(condition, msg) assert((condition) && (msg))
#else
    #define LR_ASSERT(condition) ((void)0)
    #define LR_ASSERT_MSG(condition, msg) ((void)0)
#endif

// 未使用参数
#define LR_UNUSED(x) ((void)(x))

// 数组大小
#define LR_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// 位操作
#define LR_BIT(x) (1u << (x))
#define LR_HAS_FLAG(flags, flag) (((flags) & (flag)) == (flag))
#define LR_SET_FLAG(flags, flag) ((flags) |= (flag))
#define LR_CLEAR_FLAG(flags, flag) ((flags) &= ~(flag))

// 对齐
#if defined(LR_COMPILER_MSVC)
    #define LR_ALIGN(x) __declspec(align(x))
#else
    #define LR_ALIGN(x) __attribute__((aligned(x)))
#endif

// 弃用标记
#if defined(LR_COMPILER_MSVC)
    #define LR_DEPRECATED __declspec(deprecated)
#else
    #define LR_DEPRECATED __attribute__((deprecated))
#endif

// 可能未使用
#if defined(__cplusplus) && __cplusplus >= 201703L
    #define LR_MAYBE_UNUSED [[maybe_unused]]
#else
    #define LR_MAYBE_UNUSED
#endif

// 无返回
#if defined(__cplusplus) && __cplusplus >= 201103L
    #define LR_NORETURN [[noreturn]]
#elif defined(LR_COMPILER_MSVC)
    #define LR_NORETURN __declspec(noreturn)
#else
    #define LR_NORETURN __attribute__((noreturn))
#endif

namespace lrengine {
namespace render {

// 前向声明
class LRResource;
class LRBuffer;
class LRVertexBuffer;
class LRIndexBuffer;
class LRUniformBuffer;
class LRShader;
class LRTexture;
class LRSampler;
class LRFrameBuffer;
class LRPipelineState;
class LRRenderPass;
class LRFence;
class LRRenderContext;

} // namespace render
} // namespace lrengine
