/**
 * @file LRLog.h
 * @brief LREngine日志系统
 */

#pragma once

#include "lrengine/core/LRDefines.h"

#include <functional>
#include <string>
#include <cstdarg>

namespace lrengine {
namespace utils {

/**
 * @brief 日志级别枚举
 */
enum class LogLevel : uint8_t {
    Trace = 0,    // 详细追踪（仅调试）
    Debug = 1,    // 调试信息（仅调试）
    Info = 2,     // 一般信息
    Warning = 3,  // 警告
    Error = 4,    // 错误
    Fatal = 5,    // 致命错误
    Off = 6       // 关闭日志
};

/**
 * @brief 日志条目结构
 */
struct LogEntry {
    LogLevel level;
    std::string message;
    std::string file;
    int32_t line;
    std::string function;
    uint64_t timestamp;    // 毫秒时间戳
    uint64_t threadId;
};

/**
 * @brief 日志回调函数类型
 */
using LogCallback = std::function<void(const LogEntry&)>;

/**
 * @brief 日志处理类
 */
class LR_API LRLog {
public:
    LRLog() = delete;  // 静态类
    
    /**
     * @brief 初始化日志系统
     */
    static void Initialize();
    
    /**
     * @brief 关闭日志系统
     */
    static void Shutdown();
    
    /**
     * @brief 基础日志输出
     * @param level 日志级别
     * @param message 日志消息
     */
    static void Log(LogLevel level, const char* message);
    
    /**
     * @brief 带位置信息的日志输出
     * @param level 日志级别
     * @param message 日志消息
     * @param file 源文件名
     * @param line 行号
     * @param function 函数名
     */
    static void LogEx(LogLevel level, const char* message,
                      const char* file, int32_t line, const char* function);
    
    /**
     * @brief 格式化日志输出（printf 风格）
     * @param level 日志级别
     * @param file 源文件名
     * @param line 行号
     * @param function 函数名
     * @param format 格式字符串
     * @param ... 可变参数
     */
    static void LogFormat(LogLevel level, const char* file, int32_t line, 
                          const char* function, const char* format, ...);
    
    /**
     * @brief 设置最低日志级别
     * @param level 最低级别（低于此级别的日志将被忽略）
     */
    static void SetMinLevel(LogLevel level);
    
    /**
     * @brief 获取最低日志级别
     * @return 当前最低日志级别
     */
    static LogLevel GetMinLevel();
    
    /**
     * @brief 启用/禁用控制台输出
     * @param enable 是否启用
     */
    static void EnableConsoleOutput(bool enable);
    
    /**
     * @brief 启用文件输出
     * @param filepath 日志文件路径
     */
    static void EnableFileOutput(const char* filepath);
    
    /**
     * @brief 禁用文件输出
     */
    static void DisableFileOutput();
    
    /**
     * @brief 启用/禁用彩色输出
     * @param enable 是否启用
     */
    static void EnableColorOutput(bool enable);
    
    /**
     * @brief 设置日志回调
     * @param callback 回调函数
     */
    static void SetLogCallback(LogCallback callback);
    
    /**
     * @brief 刷新日志缓冲区
     */
    static void Flush();
    
    /**
     * @brief 获取日志级别的字符串描述
     * @param level 日志级别
     * @return 级别描述字符串
     */
    static const char* GetLevelString(LogLevel level);
    
private:
    /**
     * @brief 内部格式化实现
     */
    static void LogFormatV(LogLevel level, const char* file, int32_t line,
                           const char* function, const char* format, va_list args);
};

// ============================================================================
// 基础日志宏（固定消息）
// ============================================================================
#define LR_LOG_TRACE(msg)   lrengine::utils::LRLog::LogEx(lrengine::utils::LogLevel::Trace, msg, __FILE__, __LINE__, __FUNCTION__)
#define LR_LOG_DEBUG(msg)   lrengine::utils::LRLog::LogEx(lrengine::utils::LogLevel::Debug, msg, __FILE__, __LINE__, __FUNCTION__)
#define LR_LOG_INFO(msg)    lrengine::utils::LRLog::LogEx(lrengine::utils::LogLevel::Info, msg, __FILE__, __LINE__, __FUNCTION__)
#define LR_LOG_WARNING(msg) lrengine::utils::LRLog::LogEx(lrengine::utils::LogLevel::Warning, msg, __FILE__, __LINE__, __FUNCTION__)
#define LR_LOG_ERROR(msg)   lrengine::utils::LRLog::LogEx(lrengine::utils::LogLevel::Error, msg, __FILE__, __LINE__, __FUNCTION__)
#define LR_LOG_FATAL(msg)   lrengine::utils::LRLog::LogEx(lrengine::utils::LogLevel::Fatal, msg, __FILE__, __LINE__, __FUNCTION__)

// ============================================================================
// 格式化日志宏（printf 风格，支持可变参数）
// ============================================================================
#define LR_LOG_TRACE_F(fmt, ...)   lrengine::utils::LRLog::LogFormat(lrengine::utils::LogLevel::Trace, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define LR_LOG_DEBUG_F(fmt, ...)   lrengine::utils::LRLog::LogFormat(lrengine::utils::LogLevel::Debug, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define LR_LOG_INFO_F(fmt, ...)    lrengine::utils::LRLog::LogFormat(lrengine::utils::LogLevel::Info, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define LR_LOG_WARNING_F(fmt, ...) lrengine::utils::LRLog::LogFormat(lrengine::utils::LogLevel::Warning, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define LR_LOG_ERROR_F(fmt, ...)   lrengine::utils::LRLog::LogFormat(lrengine::utils::LogLevel::Error, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define LR_LOG_FATAL_F(fmt, ...)   lrengine::utils::LRLog::LogFormat(lrengine::utils::LogLevel::Fatal, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)

// ============================================================================
// 条件编译：Release 模式下禁用 Trace/Debug
// ============================================================================
#if !defined(LR_DEBUG) && !defined(_DEBUG)
    #undef LR_LOG_TRACE
    #undef LR_LOG_DEBUG
    #undef LR_LOG_TRACE_F
    #undef LR_LOG_DEBUG_F
    #define LR_LOG_TRACE(msg)        ((void)0)
    #define LR_LOG_DEBUG(msg)        ((void)0)
    #define LR_LOG_TRACE_F(fmt, ...) ((void)0)
    #define LR_LOG_DEBUG_F(fmt, ...) ((void)0)
#endif

} // namespace utils
} // namespace lrengine
