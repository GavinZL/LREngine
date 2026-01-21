/**
 * @file LRError.cpp
 * @brief LREngine错误处理实现
 */

#include "lrengine/core/LRError.h"

#include <mutex>

namespace lrengine {
namespace render {

namespace {

// 线程局部错误信息
thread_local ErrorInfo s_lastError;

// 全局错误回调
std::mutex s_callbackMutex;
ErrorCallback s_errorCallback = nullptr;

} // anonymous namespace

ErrorCode LRError::GetLastError() { return s_lastError.code; }

const ErrorInfo& LRError::GetLastErrorInfo() { return s_lastError; }

void LRError::ClearError() { s_lastError = ErrorInfo {}; }

void LRError::SetError(ErrorCode code, const char* message, ErrorSeverity severity) {
    SetErrorEx(code, message, nullptr, 0, nullptr, severity);
}

void LRError::SetErrorEx(ErrorCode code,
                         const char* message,
                         const char* file,
                         int32_t line,
                         const char* function,
                         ErrorSeverity severity) {
    s_lastError.code     = code;
    s_lastError.severity = severity;
    s_lastError.message  = message ? message : GetErrorString(code);
    s_lastError.file     = file ? file : "";
    s_lastError.line     = line;
    s_lastError.function = function ? function : "";

    // 调用回调
    ErrorCallback callback = nullptr;
    {
        std::lock_guard<std::mutex> lock(s_callbackMutex);
        callback = s_errorCallback;
    }

    if (callback) {
        callback(s_lastError);
    }
}

const char* LRError::GetErrorString(ErrorCode code) {
    switch (code) {
        case ErrorCode::Success:
            return "Success";
        case ErrorCode::Unknown:
            return "Unknown error";
        case ErrorCode::InvalidArgument:
            return "Invalid argument";
        case ErrorCode::InvalidOperation:
            return "Invalid operation";
        case ErrorCode::OutOfMemory:
            return "Out of memory";
        case ErrorCode::NotSupported:
            return "Not supported";
        case ErrorCode::NotImplemented:
            return "Not implemented";
        case ErrorCode::NotInitialized:
            return "Not initialized";
        case ErrorCode::AlreadyInitialized:
            return "Already initialized";

        case ErrorCode::DeviceCreationFailed:
            return "Device creation failed";
        case ErrorCode::DeviceLost:
            return "Device lost";
        case ErrorCode::DeviceNotReady:
            return "Device not ready";
        case ErrorCode::ContextCreationFailed:
            return "Context creation failed";
        case ErrorCode::BackendNotAvailable:
            return "Backend not available";

        case ErrorCode::ResourceCreationFailed:
            return "Resource creation failed";
        case ErrorCode::ResourceNotFound:
            return "Resource not found";
        case ErrorCode::ResourceInUse:
            return "Resource in use";
        case ErrorCode::ResourceInvalid:
            return "Resource invalid";
        case ErrorCode::BufferMapFailed:
            return "Buffer map failed";
        case ErrorCode::BufferTooSmall:
            return "Buffer too small";

        case ErrorCode::ShaderCompileFailed:
            return "Shader compilation failed";
        case ErrorCode::ShaderLinkFailed:
            return "Shader link failed";
        case ErrorCode::ShaderNotCompiled:
            return "Shader not compiled";
        case ErrorCode::UniformNotFound:
            return "Uniform not found";
        case ErrorCode::AttributeNotFound:
            return "Attribute not found";

        case ErrorCode::TextureCreationFailed:
            return "Texture creation failed";
        case ErrorCode::TextureFormatNotSupported:
            return "Texture format not supported";
        case ErrorCode::TextureSizeExceeded:
            return "Texture size exceeded";

        case ErrorCode::FrameBufferIncomplete:
            return "FrameBuffer incomplete";
        case ErrorCode::FrameBufferAttachmentError:
            return "FrameBuffer attachment error";

        case ErrorCode::PipelineCreationFailed:
            return "Pipeline creation failed";
        case ErrorCode::PipelineStateInvalid:
            return "Pipeline state invalid";

        case ErrorCode::FenceTimeout:
            return "Fence timeout";
        case ErrorCode::FenceError:
            return "Fence error";

        case ErrorCode::FileNotFound:
            return "File not found";
        case ErrorCode::FileReadFailed:
            return "File read failed";
        case ErrorCode::FileWriteFailed:
            return "File write failed";

        default:
            return "Unknown error code";
    }
}

void LRError::SetErrorCallback(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(s_callbackMutex);
    s_errorCallback = std::move(callback);
}

bool LRError::HasError() { return s_lastError.code != ErrorCode::Success; }

} // namespace render
} // namespace lrengine
