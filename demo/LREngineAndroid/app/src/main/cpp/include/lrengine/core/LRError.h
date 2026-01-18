/**
 * @file LRError.h
 * @brief LREngine错误处理机制
 */

#pragma once

#include "LRDefines.h"

#include <functional>
#include <string>

namespace lrengine {
namespace render {

/**
 * @brief 错误码枚举
 */
enum class ErrorCode : int32_t {
    Success = 0,
    
    // 通用错误 (1-99)
    Unknown = 1,
    InvalidArgument = 2,
    InvalidOperation = 3,
    OutOfMemory = 4,
    NotSupported = 5,
    NotImplemented = 6,
    NotInitialized = 7,
    AlreadyInitialized = 8,
    InvalidState = 9,
    
    // 设备错误 (100-199)
    DeviceCreationFailed = 100,
    DeviceLost = 101,
    DeviceNotReady = 102,
    ContextCreationFailed = 103,
    BackendNotAvailable = 104,
    
    // 资源错误 (200-299)
    ResourceCreationFailed = 200,
    ResourceNotFound = 201,
    ResourceInUse = 202,
    ResourceInvalid = 203,
    BufferMapFailed = 204,
    BufferTooSmall = 205,
    
    // 着色器错误 (300-399)
    ShaderCompileFailed = 300,
    ShaderLinkFailed = 301,
    ShaderNotCompiled = 302,
    UniformNotFound = 303,
    AttributeNotFound = 304,
    
    // 纹理错误 (400-499)
    TextureCreationFailed = 400,
    TextureFormatNotSupported = 401,
    TextureSizeExceeded = 402,
    
    // 帧缓冲错误 (500-599)
    FrameBufferIncomplete = 500,
    FrameBufferAttachmentError = 501,
    
    // 管线错误 (600-699)
    PipelineCreationFailed = 600,
    PipelineStateInvalid = 601,
    
    // 同步错误 (700-799)
    FenceTimeout = 700,
    FenceError = 701,
    
    // 文件错误 (800-899)
    FileNotFound = 800,
    FileReadFailed = 801,
    FileWriteFailed = 802
};

/**
 * @brief 错误严重级别
 */
enum class ErrorSeverity : uint8_t {
    Info,       // 信息
    Warning,    // 警告
    Error,      // 错误
    Fatal       // 致命错误
};

/**
 * @brief 错误信息结构
 */
struct ErrorInfo {
    ErrorCode code = ErrorCode::Success;
    ErrorSeverity severity = ErrorSeverity::Info;
    std::string message;
    std::string file;
    int32_t line = 0;
    std::string function;
};

/**
 * @brief 错误回调函数类型
 */
using ErrorCallback = std::function<void(const ErrorInfo& error)>;

/**
 * @brief 错误处理类
 */
class LR_API LRError {
public:
    /**
     * @brief 获取最后的错误码
     * @return 最后发生的错误码
     */
    static ErrorCode GetLastError();
    
    /**
     * @brief 获取最后的错误信息
     * @return 最后的错误信息结构
     */
    static const ErrorInfo& GetLastErrorInfo();
    
    /**
     * @brief 清除错误状态
     */
    static void ClearError();
    
    /**
     * @brief 设置错误
     * @param code 错误码
     * @param message 错误消息
     * @param severity 严重级别
     */
    static void SetError(ErrorCode code, 
                        const char* message = nullptr,
                        ErrorSeverity severity = ErrorSeverity::Error);
    
    /**
     * @brief 设置错误（带位置信息）
     */
    static void SetErrorEx(ErrorCode code,
                          const char* message,
                          const char* file,
                          int32_t line,
                          const char* function,
                          ErrorSeverity severity = ErrorSeverity::Error);
    
    /**
     * @brief 获取错误码的字符串描述
     * @param code 错误码
     * @return 错误描述字符串
     */
    static const char* GetErrorString(ErrorCode code);
    
    /**
     * @brief 设置错误回调
     * @param callback 回调函数
     */
    static void SetErrorCallback(ErrorCallback callback);
    
    /**
     * @brief 检查是否有错误
     * @return 如果有错误返回true
     */
    static bool HasError();
    
    /**
     * @brief 检查错误码是否表示成功
     */
    static bool IsSuccess(ErrorCode code) {
        return code == ErrorCode::Success;
    }
    
private:
    LRError() = delete;
};

// 错误设置宏（自动填充位置信息）
#define LR_SET_ERROR(code, msg) \
    lrengine::render::LRError::SetErrorEx(code, msg, __FILE__, __LINE__, __FUNCTION__)

#define LR_SET_ERROR_SEVERITY(code, msg, severity) \
    lrengine::render::LRError::SetErrorEx(code, msg, __FILE__, __LINE__, __FUNCTION__, severity)

// 检查条件并设置错误
#define LR_CHECK(condition, code, msg) \
    do { \
        if (!(condition)) { \
            LR_SET_ERROR(code, msg); \
            return false; \
        } \
    } while(0)

#define LR_CHECK_RETURN(condition, code, msg, retval) \
    do { \
        if (!(condition)) { \
            LR_SET_ERROR(code, msg); \
            return retval; \
        } \
    } while(0)

#define LR_CHECK_NULLPTR(condition, code, msg) \
    do { \
        if (!(condition)) { \
            LR_SET_ERROR(code, msg); \
            return nullptr; \
        } \
    } while(0)

} // namespace render
} // namespace lrengine
