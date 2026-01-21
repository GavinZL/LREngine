/**
 * @file LRLog.cpp
 * @brief LREngine日志系统实现
 */

#include "lrengine/utils/LRLog.h"

#include <iostream>
#include <fstream>
#include <mutex>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>
#include <sstream>

#if defined(LR_PLATFORM_WINDOWS)
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace lrengine {
namespace utils {

namespace {
// 线程本地格式化缓冲区
thread_local char s_format_buffer[4096];

// 全局状态
std::mutex s_callback_mutex;
std::mutex s_file_mutex;
LogCallback s_log_callback = nullptr;
std::ofstream s_log_file;
LogLevel s_min_level   = LogLevel::Info;
bool s_console_enabled = true;
bool s_color_enabled   = true;
bool s_initialized     = false;

// ANSI 颜色代码
const char* GetColorCode(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:
            return "\033[90m"; // 灰色
        case LogLevel::Debug:
            return "\033[36m"; // 青色
        case LogLevel::Info:
            return "\033[32m"; // 绿色
        case LogLevel::Warning:
            return "\033[33m"; // 黄色
        case LogLevel::Error:
            return "\033[31m"; // 红色
        case LogLevel::Fatal:
            return "\033[41;97m"; // 红底白字
        default:
            return "\033[0m";
    }
}

const char* GetResetCode() { return "\033[0m"; }

// 获取当前时间戳（毫秒）
uint64_t GetTimestampMs() {
    auto now      = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

// 获取当前线程ID
uint64_t GetCurrentThreadId() {
#if defined(LR_PLATFORM_WINDOWS)
    return static_cast<uint64_t>(::GetCurrentThreadId());
#elif defined(LR_PLATFORM_APPLE)
    uint64_t tid = 0;
    pthread_threadid_np(nullptr, &tid);
    return tid;
#elif defined(LR_PLATFORM_LINUX)
    return static_cast<uint64_t>(pthread_self());
#else
    std::hash<std::thread::id> hasher;
    return static_cast<uint64_t>(hasher(std::this_thread::get_id()));
#endif
}

// 格式化时间戳为字符串
std::string FormatTimestamp(uint64_t timestamp) {
    auto ms        = timestamp % 1000;
    time_t seconds = static_cast<time_t>(timestamp / 1000);
    struct tm timeInfo;

#if defined(LR_PLATFORM_WINDOWS)
    localtime_s(&timeInfo, &seconds);
#else
    localtime_r(&seconds, &timeInfo);
#endif

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d.%03d", timeInfo.tm_year + 1900,
             timeInfo.tm_mon + 1, timeInfo.tm_mday, timeInfo.tm_hour, timeInfo.tm_min,
             timeInfo.tm_sec, static_cast<int>(ms));
    return buffer;
}

// 从完整路径提取文件名
const char* ExtractFileName(const char* path) {
    if (!path) return "";

    const char* lastSlash = path;
    for (const char* p = path; *p; ++p) {
        if (*p == '/' || *p == '\\') {
            lastSlash = p + 1;
        }
    }
    return lastSlash;
}

// 输出到控制台
void OutputToConsole(const LogEntry& entry, const std::string& formattedTime) {
    const char* levelStr = LRLog::GetLevelString(entry.level);
    const char* fileName = ExtractFileName(entry.file.c_str());

#if defined(LR_PLATFORM_WINDOWS)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    WORD colorAttr  = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

    if (s_color_enabled) {
        switch (entry.level) {
            case LogLevel::Trace:
                colorAttr = FOREGROUND_INTENSITY;
                break;
            case LogLevel::Debug:
                colorAttr = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
                break;
            case LogLevel::Info:
                colorAttr = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
                break;
            case LogLevel::Warning:
                colorAttr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
                break;
            case LogLevel::Error:
                colorAttr = FOREGROUND_RED | FOREGROUND_INTENSITY;
                break;
            case LogLevel::Fatal:
                colorAttr = BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE |
                    FOREGROUND_INTENSITY;
                break;
            default:
                break;
        }
        SetConsoleTextAttribute(hConsole, colorAttr);
    }

    fprintf(stderr, "[%s] [%-5s] [%s:%d] %s\n", formattedTime.c_str(), levelStr, fileName,
            entry.line, entry.message.c_str());

    if (s_color_enabled) {
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }
#else
    if (s_color_enabled) {
        fprintf(stderr, "%s[%s] [%-5s] [%s:%d] %s%s\n", GetColorCode(entry.level),
                formattedTime.c_str(), levelStr, fileName, entry.line, entry.message.c_str(),
                GetResetCode());
    } else {
        fprintf(stderr, "[%s] [%-5s] [%s:%d] %s\n", formattedTime.c_str(), levelStr, fileName,
                entry.line, entry.message.c_str());
    }
#endif
}

// 输出到文件
void OutputToFile(const LogEntry& entry, const std::string& formattedTime) {
    if (!s_log_file.is_open()) return;

    const char* levelStr = LRLog::GetLevelString(entry.level);
    const char* fileName = ExtractFileName(entry.file.c_str());

    std::lock_guard<std::mutex> lock(s_file_mutex);
    s_log_file << "[" << formattedTime << "] [" << levelStr << "] [" << fileName << ":"
               << entry.line << "] " << entry.message << "\n";

    // Error 及以上级别立即刷新
    if (entry.level >= LogLevel::Error) {
        s_log_file.flush();
    }
}
} // namespace

void LRLog::Initialize() {
    if (s_initialized) return;
    s_initialized     = true;
    s_min_level       = LogLevel::Info;
    s_console_enabled = true;
    s_color_enabled   = true;
}

void LRLog::Shutdown() {
    if (!s_initialized) return;

    Flush();
    DisableFileOutput();

    {
        std::lock_guard<std::mutex> lock(s_callback_mutex);
        s_log_callback = nullptr;
    }

    s_initialized = false;
}

void LRLog::Log(LogLevel level, const char* message) { LogEx(level, message, "", 0, ""); }

void LRLog::LogEx(LogLevel level,
                  const char* message,
                  const char* file,
                  int32_t line,
                  const char* function) {
    // 快速路径：级别检查
    if (level < s_min_level || level == LogLevel::Off) return;

    // 构建日志条目
    LogEntry entry;
    entry.level     = level;
    entry.message   = message ? message : "";
    entry.file      = file ? file : "";
    entry.line      = line;
    entry.function  = function ? function : "";
    entry.timestamp = GetTimestampMs();
    entry.threadId  = GetCurrentThreadId();

    std::string formattedTime = FormatTimestamp(entry.timestamp);

    // 控制台输出
    if (s_console_enabled) {
        OutputToConsole(entry, formattedTime);
    }

    // 文件输出
    OutputToFile(entry, formattedTime);

    // 回调
    LogCallback callback;
    {
        std::lock_guard<std::mutex> lock(s_callback_mutex);
        callback = s_log_callback;
    }
    if (callback) {
        callback(entry);
    }
}

void LRLog::LogFormat(LogLevel level,
                      const char* file,
                      int32_t line,
                      const char* function,
                      const char* format,
                      ...) {
    // 快速路径：级别检查
    if (level < s_min_level || level == LogLevel::Off) return;

    va_list args;
    va_start(args, format);
    LogFormatV(level, file, line, function, format, args);
    va_end(args);
}

void LRLog::LogFormatV(LogLevel level,
                       const char* file,
                       int32_t line,
                       const char* function,
                       const char* format,
                       va_list args) {
    // 使用线程本地缓冲区
    int written = vsnprintf(s_format_buffer, sizeof(s_format_buffer), format, args);

    if (written < 0) {
        s_format_buffer[0] = '\0'; // 格式化失败
    } else if (written >= static_cast<int>(sizeof(s_format_buffer))) {
        // 消息被截断，添加省略标记
        s_format_buffer[sizeof(s_format_buffer) - 4] = '.';
        s_format_buffer[sizeof(s_format_buffer) - 3] = '.';
        s_format_buffer[sizeof(s_format_buffer) - 2] = '.';
        s_format_buffer[sizeof(s_format_buffer) - 1] = '\0';
    }

    // 调用基础日志函数
    LogEx(level, s_format_buffer, file, line, function);
}

void LRLog::SetMinLevel(LogLevel level) { s_min_level = level; }

LogLevel LRLog::GetMinLevel() { return s_min_level; }

void LRLog::EnableConsoleOutput(bool enable) { s_console_enabled = enable; }

void LRLog::EnableFileOutput(const char* filepath) {
    std::lock_guard<std::mutex> lock(s_file_mutex);

    if (s_log_file.is_open()) {
        s_log_file.close();
    }

    s_log_file.open(filepath, std::ios::out | std::ios::app);
}

void LRLog::DisableFileOutput() {
    std::lock_guard<std::mutex> lock(s_file_mutex);

    if (s_log_file.is_open()) {
        s_log_file.flush();
        s_log_file.close();
    }
}

void LRLog::EnableColorOutput(bool enable) { s_color_enabled = enable; }

void LRLog::SetLogCallback(LogCallback callback) {
    std::lock_guard<std::mutex> lock(s_callback_mutex);
    s_log_callback = std::move(callback);
}

void LRLog::Flush() {
    fflush(stderr);

    std::lock_guard<std::mutex> lock(s_file_mutex);
    if (s_log_file.is_open()) {
        s_log_file.flush();
    }
}

const char* LRLog::GetLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:
            return "TRACE";
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warning:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
        case LogLevel::Fatal:
            return "FATAL";
        case LogLevel::Off:
            return "OFF";
        default:
            return "UNKNOWN";
    }
}

} // namespace utils
} // namespace lrengine
