/**
 * @file TestLRLog.cpp
 * @brief LRLog 日志系统单元测试
 */

#include "lrengine/utils/LRLog.h"

#include <iostream>
#include <vector>
#include <thread>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <atomic>

using namespace lrengine::utils;

// 测试计数器
static int s_tests_passed = 0;
static int s_tests_failed = 0;

#define TEST_ASSERT(condition, msg) \
    do { \
        if (condition) { \
            s_tests_passed++; \
            std::cout << "[PASS] " << msg << std::endl; \
        } else { \
            s_tests_failed++; \
            std::cerr << "[FAIL] " << msg << " (line " << __LINE__ << ")" << std::endl; \
        } \
    } while(0)

// ============================================================================
// 测试用例
// ============================================================================

void TestLogLevelString() {
    std::cout << "\n=== Test: LogLevel String ===" << std::endl;
    
    TEST_ASSERT(strcmp(LRLog::GetLevelString(LogLevel::Trace), "TRACE") == 0, "Trace level string");
    TEST_ASSERT(strcmp(LRLog::GetLevelString(LogLevel::Debug), "DEBUG") == 0, "Debug level string");
    TEST_ASSERT(strcmp(LRLog::GetLevelString(LogLevel::Info), "INFO") == 0, "Info level string");
    TEST_ASSERT(strcmp(LRLog::GetLevelString(LogLevel::Warning), "WARN") == 0, "Warning level string");
    TEST_ASSERT(strcmp(LRLog::GetLevelString(LogLevel::Error), "ERROR") == 0, "Error level string");
    TEST_ASSERT(strcmp(LRLog::GetLevelString(LogLevel::Fatal), "FATAL") == 0, "Fatal level string");
    TEST_ASSERT(strcmp(LRLog::GetLevelString(LogLevel::Off), "OFF") == 0, "Off level string");
}

void TestLogLevelFilter() {
    std::cout << "\n=== Test: LogLevel Filter ===" << std::endl;
    
    // 记录回调收到的日志数量
    int logCount = 0;
    
    LRLog::Initialize();
    LRLog::EnableConsoleOutput(false);  // 禁用控制台，只通过回调测试
    
    LRLog::SetLogCallback([&logCount](const LogEntry& entry) {
        logCount++;
    });
    
    // 设置最低级别为 Warning
    LRLog::SetMinLevel(LogLevel::Warning);
    TEST_ASSERT(LRLog::GetMinLevel() == LogLevel::Warning, "SetMinLevel to Warning");
    
    logCount = 0;
    LRLog::Log(LogLevel::Trace, "trace");
    LRLog::Log(LogLevel::Debug, "debug");
    LRLog::Log(LogLevel::Info, "info");
    LRLog::Log(LogLevel::Warning, "warning");
    LRLog::Log(LogLevel::Error, "error");
    LRLog::Log(LogLevel::Fatal, "fatal");
    
    TEST_ASSERT(logCount == 3, "Only Warning/Error/Fatal should pass (got " + std::to_string(logCount) + ")");
    
    // 设置为 Trace，所有日志都应该通过
    LRLog::SetMinLevel(LogLevel::Trace);
    logCount = 0;
    LRLog::Log(LogLevel::Trace, "trace");
    LRLog::Log(LogLevel::Debug, "debug");
    LRLog::Log(LogLevel::Info, "info");
    LRLog::Log(LogLevel::Warning, "warning");
    LRLog::Log(LogLevel::Error, "error");
    LRLog::Log(LogLevel::Fatal, "fatal");
    
    TEST_ASSERT(logCount == 6, "All levels should pass when MinLevel is Trace (got " + std::to_string(logCount) + ")");
    
    // 设置为 Off，没有日志通过
    LRLog::SetMinLevel(LogLevel::Off);
    logCount = 0;
    LRLog::Log(LogLevel::Fatal, "fatal");
    TEST_ASSERT(logCount == 0, "No logs should pass when MinLevel is Off");
    
    LRLog::SetLogCallback(nullptr);
    LRLog::EnableConsoleOutput(true);
    LRLog::Shutdown();
}

void TestLogCallback() {
    std::cout << "\n=== Test: Log Callback ===" << std::endl;
    
    LRLog::Initialize();
    LRLog::EnableConsoleOutput(false);
    LRLog::SetMinLevel(LogLevel::Info);
    
    LogEntry capturedEntry;
    bool callbackCalled = false;
    
    LRLog::SetLogCallback([&](const LogEntry& entry) {
        capturedEntry = entry;
        callbackCalled = true;
    });
    
    LRLog::Log(LogLevel::Error, "Test error message");
    
    TEST_ASSERT(callbackCalled, "Callback was called");
    TEST_ASSERT(capturedEntry.level == LogLevel::Error, "Captured correct level");
    TEST_ASSERT(capturedEntry.message == "Test error message", "Captured correct message");
    TEST_ASSERT(capturedEntry.timestamp > 0, "Timestamp is set");
    TEST_ASSERT(capturedEntry.threadId > 0, "ThreadId is set");
    
    LRLog::SetLogCallback(nullptr);
    LRLog::EnableConsoleOutput(true);
    LRLog::Shutdown();
}

void TestLogFormat() {
    std::cout << "\n=== Test: Log Format ===" << std::endl;
    
    LRLog::Initialize();
    LRLog::EnableConsoleOutput(false);
    LRLog::SetMinLevel(LogLevel::Trace);
    
    std::string capturedMessage;
    
    LRLog::SetLogCallback([&](const LogEntry& entry) {
        capturedMessage = entry.message;
    });
    
    // 测试整数格式化
    LRLog::LogFormat(LogLevel::Info, __FILE__, __LINE__, __FUNCTION__, "Value: %d", 42);
    TEST_ASSERT(capturedMessage == "Value: 42", "Integer format");
    
    // 测试浮点数格式化
    LRLog::LogFormat(LogLevel::Info, __FILE__, __LINE__, __FUNCTION__, "Float: %.2f", 3.14159);
    TEST_ASSERT(capturedMessage == "Float: 3.14", "Float format");
    
    // 测试字符串格式化
    LRLog::LogFormat(LogLevel::Info, __FILE__, __LINE__, __FUNCTION__, "String: %s", "hello");
    TEST_ASSERT(capturedMessage == "String: hello", "String format");
    
    // 测试十六进制格式化
    LRLog::LogFormat(LogLevel::Info, __FILE__, __LINE__, __FUNCTION__, "Hex: 0x%04X", 0x1234);
    TEST_ASSERT(capturedMessage == "Hex: 0x1234", "Hex format");
    
    // 测试多参数格式化
    LRLog::LogFormat(LogLevel::Info, __FILE__, __LINE__, __FUNCTION__, "%s: %d x %d = %d", "Multiply", 3, 4, 12);
    TEST_ASSERT(capturedMessage == "Multiply: 3 x 4 = 12", "Multiple parameters");
    
    LRLog::SetLogCallback(nullptr);
    LRLog::EnableConsoleOutput(true);
    LRLog::Shutdown();
}

void TestLogMacros() {
    std::cout << "\n=== Test: Log Macros ===" << std::endl;
    
    LRLog::Initialize();
    LRLog::EnableConsoleOutput(false);
    LRLog::SetMinLevel(LogLevel::Trace);
    
    LogLevel capturedLevel = LogLevel::Off;
    std::string capturedMessage;
    int capturedLine = 0;
    
    LRLog::SetLogCallback([&](const LogEntry& entry) {
        capturedLevel = entry.level;
        capturedMessage = entry.message;
        capturedLine = entry.line;
    });
    
    // 测试基础宏
    LR_LOG_INFO("Info message");
    TEST_ASSERT(capturedLevel == LogLevel::Info, "LR_LOG_INFO level");
    TEST_ASSERT(capturedMessage == "Info message", "LR_LOG_INFO message");
    TEST_ASSERT(capturedLine > 0, "LR_LOG_INFO captures line number");
    
    LR_LOG_WARNING("Warning message");
    TEST_ASSERT(capturedLevel == LogLevel::Warning, "LR_LOG_WARNING level");
    
    LR_LOG_ERROR("Error message");
    TEST_ASSERT(capturedLevel == LogLevel::Error, "LR_LOG_ERROR level");
    
    // 测试格式化宏
    LR_LOG_INFO_F("Count: %d", 100);
    TEST_ASSERT(capturedMessage == "Count: 100", "LR_LOG_INFO_F format");
    
    LR_LOG_ERROR_F("Error code: 0x%08X", 0xDEADBEEF);
    TEST_ASSERT(capturedMessage == "Error code: 0xDEADBEEF", "LR_LOG_ERROR_F hex format");
    
    LRLog::SetLogCallback(nullptr);
    LRLog::EnableConsoleOutput(true);
    LRLog::Shutdown();
}

void TestFileOutput() {
    std::cout << "\n=== Test: File Output ===" << std::endl;
    
    const char* testLogFile = "test_log.txt";
    
    // 删除已存在的测试文件
    std::remove(testLogFile);
    
    LRLog::Initialize();
    LRLog::EnableConsoleOutput(false);
    LRLog::SetMinLevel(LogLevel::Info);
    LRLog::EnableFileOutput(testLogFile);
    
    LRLog::Log(LogLevel::Info, "File output test");
    LRLog::Log(LogLevel::Error, "Error in file");
    
    LRLog::Flush();
    LRLog::DisableFileOutput();
    LRLog::Shutdown();
    
    // 验证文件内容
    std::ifstream file(testLogFile);
    TEST_ASSERT(file.is_open(), "Log file was created");
    
    if (file.is_open()) {
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        TEST_ASSERT(content.find("File output test") != std::string::npos, "File contains first message");
        TEST_ASSERT(content.find("Error in file") != std::string::npos, "File contains second message");
        TEST_ASSERT(content.find("[INFO]") != std::string::npos, "File contains INFO level");
        TEST_ASSERT(content.find("[ERROR]") != std::string::npos, "File contains ERROR level");
    }
    
    // 清理测试文件
    std::remove(testLogFile);
}

void TestThreadSafety() {
    std::cout << "\n=== Test: Thread Safety ===" << std::endl;
    
    LRLog::Initialize();
    LRLog::EnableConsoleOutput(false);
    LRLog::SetMinLevel(LogLevel::Info);
    
    std::atomic<int> logCount{0};
    
    LRLog::SetLogCallback([&](const LogEntry& entry) {
        logCount++;
    });
    
    const int numThreads = 4;
    const int logsPerThread = 100;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([t, logsPerThread]() {
            for (int i = 0; i < logsPerThread; ++i) {
                LR_LOG_INFO_F("Thread %d, Log %d", t, i);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    int expected = numThreads * logsPerThread;
    TEST_ASSERT(logCount == expected, 
        "All logs received from multiple threads (expected " + std::to_string(expected) + 
        ", got " + std::to_string(logCount.load()) + ")");
    
    LRLog::SetLogCallback(nullptr);
    LRLog::EnableConsoleOutput(true);
    LRLog::Shutdown();
}

void TestInitializeShutdown() {
    std::cout << "\n=== Test: Initialize/Shutdown ===" << std::endl;
    
    // 多次初始化应该安全
    LRLog::Initialize();
    LRLog::Initialize();
    TEST_ASSERT(true, "Multiple Initialize calls are safe");
    
    LRLog::Shutdown();
    LRLog::Shutdown();
    TEST_ASSERT(true, "Multiple Shutdown calls are safe");
    
    // 再次初始化应该工作
    LRLog::Initialize();
    LRLog::SetMinLevel(LogLevel::Debug);
    TEST_ASSERT(LRLog::GetMinLevel() == LogLevel::Debug, "Re-initialize works");
    LRLog::Shutdown();
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "LRLog Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    TestLogLevelString();
    TestLogLevelFilter();
    TestLogCallback();
    TestLogFormat();
    TestLogMacros();
    TestFileOutput();
    TestThreadSafety();
    TestInitializeShutdown();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Results: " << s_tests_passed << " passed, " << s_tests_failed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return s_tests_failed > 0 ? 1 : 0;
}
