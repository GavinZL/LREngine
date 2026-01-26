/**
 * @file LRPlanarTexture.h
 * @brief LREngine 多平面纹理对象，支持 YUV/NV12/NV21 等格式
 */

#pragma once

#include "LRResource.h"
#include "LRTexture.h"
#include "LRTypes.h"

#include <vector>
#include <cstdint>

namespace lrengine {
namespace render {

// 前向声明
class LRRenderContext;

/**
 * @brief 平面纹理格式
 */
enum class PlanarFormat : uint8_t {
    YUV420P,    // 3平面: Y(R8) + U(R8) + V(R8)
    NV12,       // 2平面: Y(R8) + UV(RG8)
    NV21,       // 2平面: Y(R8) + VU(RG8)
    RGBA        // 1平面: RGBA8（兼容模式）
};

/**
 * @brief 平面纹理描述符
 */
struct PlanarTextureDescriptor {
    uint32_t width = 0;
    uint32_t height = 0;
    PlanarFormat format = PlanarFormat::NV12;
    SamplerDescriptor sampler;
    const char* debugName = nullptr;
};

/**
 * @brief 多平面纹理类
 * 
 * 用于处理 YUV 等需要多个纹理对象的数据格式。
 * 每个平面对应一个独立的纹理对象。
 * 
 * 使用示例：
 * @code
 * PlanarTextureDescriptor desc;
 * desc.width = 1920;
 * desc.height = 1080;
 * desc.format = PlanarFormat::NV12;
 * 
 * auto* texture = context->CreatePlanarTexture(desc);
 * texture->UpdateAllPlanes({yData, uvData});
 * texture->BindAll(0);  // 绑定到纹理单元 0 和 1
 * @endcode
 */
class LR_API LRPlanarTexture : public LRResource {
public:
    LR_NONCOPYABLE(LRPlanarTexture);
    
    virtual ~LRPlanarTexture();
    
    // =========================================================================
    // 平面访问
    // =========================================================================
    
    /**
     * @brief 获取平面数量
     */
    uint32_t GetPlaneCount() const { return static_cast<uint32_t>(mPlanes.size()); }
    
    /**
     * @brief 获取指定平面的纹理
     * @param planeIndex 平面索引 (0=Y, 1=U/UV, 2=V)
     * @return 纹理指针，索引无效时返回 nullptr
     */
    LRTexture* GetPlaneTexture(uint32_t planeIndex) const;
    
    /**
     * @brief 获取所有平面纹理
     */
    const std::vector<LRTexture*>& GetAllPlanes() const { return mPlanes; }
    
    // =========================================================================
    // 数据更新
    // =========================================================================
    
    /**
     * @brief 更新指定平面的数据
     * @param planeIndex 平面索引
     * @param data 像素数据
     * @param stride 行字节数（0表示紧密排列）
     */
    void UpdatePlaneData(uint32_t planeIndex, const void* data, uint32_t stride = 0);
    
    /**
     * @brief 批量更新所有平面数据
     * @param planeData 各平面数据指针数组
     * @param strides 各平面行字节数数组（可选）
     */
    void UpdateAllPlanes(const std::vector<const void*>& planeData,
                        const std::vector<uint32_t>& strides = {});

// ==================== 新增：阶段 1 接口 ====================
public:
    /**
     * @brief 纹理更新配置选项
     */
    struct UpdateFromImageOptions {
        bool generateMipmaps = false;       ///< 是否生成 mipmap
        bool flipVertically = false;        ///< 是否垂直翻转
        TextureRegion* targetRegion = nullptr;  ///< 目标区域（nullptr 表示整个纹理）
        
        UpdateFromImageOptions() = default;
    };
    
    /**
     * @brief 从 CPU 图像数据更新纹理（使用默认选项）
     * @param imageData 图像数据描述
     * @return 是否成功
     */
    bool UpdateFromImage(const ImageDataDesc& imageData) {
        return UpdateFromImage(imageData, UpdateFromImageOptions());
    }

    /**
     * @brief 从 CPU 图像数据更新纹理
     * @param imageData 图像数据描述（包含格式、平面数据等）
     * @param options 更新选项
     * @return 是否成功
     */
    bool UpdateFromImage(const ImageDataDesc& imageData,
                        const UpdateFromImageOptions& options);

    /**
     * @brief 回读配置选项
     */
    struct ReadbackOptions {
        ImageFormat targetFormat = ImageFormat::Unknown;  ///< 目标格式（Unknown 表示保持原格式）
        ColorSpace targetColorSpace = ColorSpace::Unknown;
        ColorRange targetColorRange = ColorRange::Unknown;
        bool asyncReadback = true;          ///< 是否异步回读
        void* userContext = nullptr;        ///< 用户上下文指针
        
        ReadbackOptions() = default;
    };
    
    /**
     * @brief 回读结果
     */
    struct ReadbackResult {
        bool success = false;
        ImageDataDesc imageData;            ///< 回读的图像数据
        void* nativeBuffer = nullptr;       ///< 平台原生缓冲区（CVPixelBuffer* / AHardwareBuffer*）
        
        ReadbackResult() = default;
    };
    
    /**
     * @brief 从 GPU 纹理回读数据到 CPU（使用默认选项）
     * @param outResult 回读结果
     * @return 是否成功发起回读
     */
    bool Readback(ReadbackResult& outResult) {
        return Readback(outResult, ReadbackOptions());
    }

    /**
     * @brief 从 GPU 纹理回读数据到 CPU
     * @param outResult 回读结果（包含图像数据或原生缓冲区）
     * @param options 回读选项
     * @return 是否成功发起回读
     */
    bool Readback(ReadbackResult& outResult, const ReadbackOptions& options);
    
    // =========================================================================
    // 纹理绑定
    // =========================================================================
    
    /**
     * @brief 绑定所有平面到连续纹理单元
     * @param baseSlot 起始纹理单元索引
     */
    void BindAll(uint32_t baseSlot = 0);
    
    /**
     * @brief 解绑所有平面
     */
    void UnbindAll();
    
    /**
     * @brief 绑定指定平面
     * @param planeIndex 平面索引
     * @param slot 纹理单元索引
     */
    void BindPlane(uint32_t planeIndex, uint32_t slot);
    
    /**
     * @brief 解绑指定平面
     * @param planeIndex 平面索引
     * @param slot 纹理单元索引
     */
    void UnbindPlane(uint32_t planeIndex, uint32_t slot);
    
    // =========================================================================
    // 属性获取
    // =========================================================================
    
    /**
     * @brief 获取纹理宽度
     */
    uint32_t GetWidth() const { return mWidth; }
    
    /**
     * @brief 获取纹理高度
     */
    uint32_t GetHeight() const { return mHeight; }
    
    /**
     * @brief 获取内部平面布局格式
     */
    PlanarFormat GetFormat() const { return mFormat; }
    
    /**
     * @brief 获取对外图像格式（与 UpdateFromImage 对应）
     */
    ImageFormat GetImageFormat() const { return mImageFormat; }
    
    /**
     * @brief 获取指定平面的尺寸
     * @param planeIndex 平面索引
     * @param[out] width 宽度
     * @param[out] height 高度
     */
    void GetPlaneSize(uint32_t planeIndex, uint32_t& width, uint32_t& height) const;
    
    /**
     * @brief 获取指定平面的像素格式
     * @param planeIndex 平面索引
     * @return 像素格式
     */
    PixelFormat GetPlaneFormat(uint32_t planeIndex) const;
    
    /**
     * @brief 获取原生句柄（默认返回第一个平面的句柄）
     */
    ResourceHandle GetNativeHandle() const override;
    
    /**
     * @brief 获取指定平面的原生句柄
     * @param planeIndex 平面索引
     */
    ResourceHandle GetNativeHandle(uint32_t planeIndex) const;
    
protected:
    friend class LRRenderContext;
    
    /**
     * @brief 构造函数（仅 LRRenderContext 可调用）
     */
    LRPlanarTexture();
    
    /**
     * @brief 初始化多平面纹理
     * @param context 渲染上下文
     * @param desc 描述符
     * @return 初始化是否成功
     */
    bool Initialize(LRRenderContext* context, const PlanarTextureDescriptor& desc);
    
private:
    /**
     * @brief 根据格式创建平面纹理
     * @param desc 描述符
     * @return 创建是否成功
     */
    bool createPlanes(const PlanarTextureDescriptor& desc);
    
    /**
     * @brief 释放所有平面纹理
     */
    void releasePlanes();
    
private:
    LRRenderContext* mContext = nullptr;
    std::vector<LRTexture*> mPlanes;
    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    PlanarFormat mFormat = PlanarFormat::NV12;
    ImageFormat mImageFormat = ImageFormat::Unknown;
    uint32_t mBoundBaseSlot = 0;
};

// 类型别名
using LRPlanarTexturePtr = LRResourcePtr<LRPlanarTexture>;

} // namespace render
} // namespace lrengine
