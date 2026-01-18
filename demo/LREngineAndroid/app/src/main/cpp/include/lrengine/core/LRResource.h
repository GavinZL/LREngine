/**
 * @file LRResource.h
 * @brief LREngine渲染资源基类
 */

#pragma once

#include "LRDefines.h"
#include "LRTypes.h"

#include <atomic>
#include <string>

namespace lrengine {
namespace render {

/**
 * @brief 渲染资源基类
 * 
 * 所有GPU资源的共同基类，提供:
 * - 引用计数管理
 * - 资源ID
 * - 资源类型
 * - 调试名称
 */
class LR_API LRResource {
public:
    LR_NONCOPYABLE(LRResource);
    
    virtual ~LRResource();
    
    /**
     * @brief 增加引用计数
     */
    void AddRef();
    
    /**
     * @brief 减少引用计数，当计数为0时销毁资源
     */
    void Release();
    
    /**
     * @brief 获取当前引用计数
     */
    uint32_t GetRefCount() const;
    
    /**
     * @brief 获取资源唯一ID
     */
    uint64_t GetResourceID() const { return mResourceID; }
    
    /**
     * @brief 获取资源类型
     */
    ResourceType GetResourceType() const { return mResourceType; }
    
    /**
     * @brief 获取平台原生句柄
     */
    virtual ResourceHandle GetNativeHandle() const = 0;
    
    /**
     * @brief 检查资源是否有效
     */
    virtual bool IsValid() const;
    
    /**
     * @brief 设置调试名称
     */
    void SetDebugName(const char* name);
    
    /**
     * @brief 获取调试名称
     */
    const std::string& GetDebugName() const { return mDebugName; }
    
protected:
    /**
     * @brief 构造函数（仅派生类可调用）
     * @param type 资源类型
     */
    explicit LRResource(ResourceType type);
    
    /**
     * @brief 移动构造函数
     */
    LRResource(LRResource&& other) noexcept;
    
    /**
     * @brief 移动赋值运算符
     */
    LRResource& operator=(LRResource&& other) noexcept;
    
    /**
     * @brief 生成唯一资源ID
     */
    static uint64_t GenerateResourceID();
    
protected:
    uint64_t mResourceID;                    // 资源唯一ID
    ResourceType mResourceType;               // 资源类型
    std::atomic<uint32_t> mRefCount{1};      // 引用计数
    std::string mDebugName;                  // 调试名称
    bool mIsValid = false;                   // 有效标志
};

/**
 * @brief 资源智能指针辅助类
 * 
 * 自动管理资源引用计数
 */
template<typename T>
class LRResourcePtr {
    static_assert(std::is_base_of<LRResource, T>::value, 
                  "T must be derived from LRResource");
public:
    LRResourcePtr() : mPtr(nullptr) {}
    
    explicit LRResourcePtr(T* ptr) : mPtr(ptr) {
        // 构造时不增加引用计数（假设资源创建时已经是1）
    }
    
    LRResourcePtr(const LRResourcePtr& other) : mPtr(other.mPtr) {
        if (mPtr) {
            mPtr->AddRef();
        }
    }
    
    LRResourcePtr(LRResourcePtr&& other) noexcept : mPtr(other.mPtr) {
        other.mPtr = nullptr;
    }
    
    ~LRResourcePtr() {
        if (mPtr) {
            mPtr->Release();
        }
    }
    
    LRResourcePtr& operator=(const LRResourcePtr& other) {
        if (this != &other) {
            if (mPtr) {
                mPtr->Release();
            }
            mPtr = other.mPtr;
            if (mPtr) {
                mPtr->AddRef();
            }
        }
        return *this;
    }
    
    LRResourcePtr& operator=(LRResourcePtr&& other) noexcept {
        if (this != &other) {
            if (mPtr) {
                mPtr->Release();
            }
            mPtr = other.mPtr;
            other.mPtr = nullptr;
        }
        return *this;
    }
    
    LRResourcePtr& operator=(T* ptr) {
        if (mPtr != ptr) {
            if (mPtr) {
                mPtr->Release();
            }
            mPtr = ptr;
            // 不增加引用计数
        }
        return *this;
    }
    
    T* Get() const { return mPtr; }
    T* operator->() const { return mPtr; }
    T& operator*() const { return *mPtr; }
    
    explicit operator bool() const { return mPtr != nullptr; }
    
    bool operator==(const LRResourcePtr& other) const { return mPtr == other.mPtr; }
    bool operator!=(const LRResourcePtr& other) const { return mPtr != other.mPtr; }
    bool operator==(std::nullptr_t) const { return mPtr == nullptr; }
    bool operator!=(std::nullptr_t) const { return mPtr != nullptr; }
    
    void Reset() {
        if (mPtr) {
            mPtr->Release();
            mPtr = nullptr;
        }
    }
    
    T* Detach() {
        T* ptr = mPtr;
        mPtr = nullptr;
        return ptr;
    }
    
private:
    T* mPtr;
};

// 常用资源指针类型别名
using LRBufferPtr = LRResourcePtr<LRBuffer>;
using LRVertexBufferPtr = LRResourcePtr<LRVertexBuffer>;
using LRIndexBufferPtr = LRResourcePtr<LRIndexBuffer>;
using LRUniformBufferPtr = LRResourcePtr<LRUniformBuffer>;
using LRShaderPtr = LRResourcePtr<LRShader>;
using LRTexturePtr = LRResourcePtr<LRTexture>;
using LRSamplerPtr = LRResourcePtr<LRSampler>;
using LRFrameBufferPtr = LRResourcePtr<LRFrameBuffer>;
using LRPipelineStatePtr = LRResourcePtr<LRPipelineState>;
using LRFencePtr = LRResourcePtr<LRFence>;

} // namespace render
} // namespace lrengine
