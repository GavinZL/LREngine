好的，那我先把所有思路收敛成一份**可执行的重构计划**，完全按“新接口为主、无需兼容旧接口”的前提来设计。你确认后，我们再按这个计划一步步改代码。

---

## 1. 目标与范围

- **统一对外纹理类型**：对外只提供 **`LRPlanarTexture`** 作为纹理类入口，负责单/多平面格式；`LRTexture` 下沉为内部单平面实现细节。
- **统一数据更新入口**：外部只通过 `LRPlanarTexture::UpdateFromImage(...)`（或等价接口）完成 CPU→GPU 更新，自动处理各种 `ImageFormat`。
- **统一原生句柄访问**：提供 `GetNativeHandle(int planeIndex)` 风格接口，按平面索引获取原生 GPU 纹理（Metal / GLES / GL / Vulkan）。
- **读回职责收敛**：移除旧式 `readTexture` CPU 输出接口，把 GPU→CPU 的数据承载和复用交给 **utils 下的对象池**。
- **平台异步零拷贝回读**：Apple 平台优先走 `CVPixelBuffer` / `CVMetalTexture`，Android 优先走 `AHardwareBuffer` / PBO；读回接口抽象统一。
- **内存池管理**：通过 `LREngine/utils` 中的 buffer pool 工具类，按格式/尺寸池化管理回读用内存对象。

---

## 2. 公共类型与接口统一设计

### 2.1 基础类型扩展（core 层）

**在 `LRTypes.h` 新增：**

- **图像格式枚举：**

```cpp
enum class ImageFormat : uint8_t {
    // YUV 系列
    YUV420P,    // 3 平面: Y + U + V
    NV12,       // 2 平面: Y + UV
    NV21,       // 2 平面: Y + VU

    // RGBA 系列
    RGBA8,
    BGRA8,
    RGB8,

    // 灰度
    GRAY8,

    UNKNOWN
};
```

- **颜色空间/范围：**

```cpp
enum class ColorSpace : uint8_t {
    BT709,
    BT601,
    BT2020,
    Unknown
};

enum class ColorRange : uint8_t {
    Video,   // Limited
    Full,
    Unknown
};
```

- **输入图像描述结构：**

```cpp
struct ImagePlaneDesc {
    const void* data = nullptr;
    uint32_t    stride = 0;   // 以字节为单位，0 表示紧密排列
};

struct ImageDataDesc {
    uint32_t    width  = 0;
    uint32_t    height = 0;
    ImageFormat format = ImageFormat::NV12;

    std::vector<ImagePlaneDesc> planes;

    ColorSpace  colorSpace = ColorSpace::BT709;
    ColorRange  range      = ColorRange::Video;
};
```

> 说明：后续所有 CPU→GPU 更新、GPU→CPU 读回接口都围绕 `ImageFormat + ImageDataDesc` 进行。

---

### 2.2 `LRTexture`（内部单平面纹理）

定位：**只作为 LRPlanarTexture 的内部组件**，不再面向外部业务暴露。接口可适当地“收紧”。

**保留/关键接口：**

```cpp
class LRTexture : public LRResource {
public:
    virtual ~LRTexture();

    void UpdateData(const void* data, const TextureRegion* region = nullptr);
    void GenerateMipmaps();

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    // ...

    // 单平面原生句柄
    ResourceHandle GetNativeHandle() const override;

protected:
    friend class LRRenderContext;
    friend class LRPlanarTexture;      // LRPlanarTexture 需要访问 Initialize

    LRTexture();
    bool Initialize(ITextureImpl* impl, const TextureDescriptor& desc);

    // ...
};
```

**对外约束：**

- 不再推荐外部模块直接拿 `LRTexture*`，统一通过 `LRPlanarTexture` 交互。
- 新代码中，不再新增直接依赖 `LRTexture` 的公共接口。

---

### 2.3 `LRPlanarTexture`（对外唯一纹理接口）

将 `LRPlanarTexture` 作为**对外唯一的纹理对象**，支持单平面（RGBA/GRAY）和多平面（NV12/YUV420 等）。

**接口重构方向：**

```cpp
class LRPlanarTexture : public LRResource {
public:
    LR_NONCOPYABLE(LRPlanarTexture);
    virtual ~LRPlanarTexture();

    // ========= 平面访问 =========
    uint32_t        GetPlaneCount() const;
    LRTexture*      GetPlaneTexture(uint32_t planeIndex) const;
    const std::vector<LRTexture*>& GetAllPlanes() const;

    // ========= 核心：CPU→GPU 更新 =========
    // - 外部统一通过这个接口更新纹理数据，不再直接操作 LRTexture
    bool UpdateFromImage(const ImageDataDesc& image);

    // ========= 属性 =========
    uint32_t    GetWidth() const;
    uint32_t    GetHeight() const;
    PlanarFormat GetPlanarFormat() const; // 内部平面布局：NV12/YUV420P/RGBA...
    ImageFormat  GetImageFormat() const;  // 对外逻辑格式（与UpdateFromImage保持一致）

    // ========= 原生句柄（核心变更） =========
    // 1) 对外统一接口：按平面索引访问底层 GPU 纹理
    ResourceHandle GetNativeHandle(int planeIndex = 0) const override;

    // ========= GPU→CPU 回读 =========
    // 注意：**不再提供 readTexture 返回裸 CPU buffer 的旧接口**
    // 新接口统一使用 utils 中的对象池来管理回读缓冲
    bool ReadbackAsync(const ReadbackOptions& options,
                       ReadbackCallback callback);

    // 若需要同步，可以封装成调用 ReadbackAsync + 阻塞等待（实现上再细化）
};
```

> 关键变化：
> - 对外只有 `GetNativeHandle(int planeIndex)`，不再单纯“默认第一个平面”；旧的无参版本可以保留为默认参数语法糖。
> - 不再提供“直接返回 `std::vector<uint8_t>` 的 readTexture”，读回的数据通过对象池返回。

---

## 3. GPU→CPU 读回接口与平台特性

根据你最新的精简设计要求，**CPU 格式输出的细节交给 utils 的对象池来管理**，核心纹理类只负责“触发读回 + 返回池化对象/句柄”。

### 3.1 统一 Readback 抽象

**在 core 层定义：**

```cpp
// utils 前置声明
namespace lrengine { namespace utils {
    class ImageBuffer;
    using ImageBufferPtr = std::shared_ptr<ImageBuffer>;
}}

struct ReadbackOptions {
    ImageFormat   targetFormat      = ImageFormat::RGBA8;
    ColorSpace    targetColorSpace  = ColorSpace::BT709;
    ColorRange    targetRange       = ColorRange::Video;

    bool          preferZeroCopy    = true;     // 尽量使用 CVPixelBuffer/AHardwareBuffer 等
    void*         externalTarget    = nullptr;  // 平台外部提供的目标，如 CVPixelBuffer*
};

struct ReadbackResult {
    // 回读结果统一用 ImageBuffer 承载
    lrengine::utils::ImageBufferPtr buffer;
};

using ReadbackCallback = std::function<void(bool success, ReadbackResult&& result)>;

class LRPlanarTexture {
public:
    bool ReadbackAsync(const ReadbackOptions& options,
                       ReadbackCallback callback);
};
```

### 3.2 iOS/macOS 路径（CVPixelBuffer & CVMetalTexture）

**设计原则：**

- 优先使用调用方提供的 `CVPixelBufferRef` 做“零拷贝 + 异步”：
  - `options.externalTarget = cvPixelBuffer;`
  - `options.preferZeroCopy = true;`

**执行流程：**

1. 从 `ImageBufferPool` 中获取一个 **封装 CVPixelBuffer 的 ImageBuffer** 或直接使用 `externalTarget`。
2. 使用 `CVMetalTextureCacheCreateTextureFromImage` 把 `CVPixelBuffer` 变成 `CVMetalTexture` / `id<MTLTexture>`。
3. 在 GPU 上通过 blit/compute，将 `LRPlanarTexture` 对应的 plane 纹理拷贝到该 Metal 纹理。
4. 提交 `MTLCommandBuffer`，在 `addCompletedHandler` 里回调 `ReadbackCallback`，并把封装好的 `ImageBufferPtr` 传回去。

> 在这个模式下，CPU 不必深拷贝像素数据，用户可以直接用 `CVPixelBuffer` 做编码/展示。

### 3.3 Android 路径（AHardwareBuffer / PBO）

**零拷贝（优先）：**

- 外部提供 `AHardwareBuffer*` / `HardwareBuffer` 句柄，通过 `externalTarget` 传入；
- Backend 把这个 buffer 导入为 `EGLImage`/`VkImage`，GPU 写入后回调；
- `ImageBuffer` 封装 `AHardwareBuffer*` 以及 plane 信息，供 CPU 映射使用或交回给平台。

**非零拷贝（CPU buffer）：**

- 后端从 `ImageBufferPool` 获取一个 **纯 CPU 内存的 ImageBuffer**（例如 `CpuImageBuffer`）；
- 用 FBO + `glReadPixels`（可配合 PBO 异步）写入 buffer；
- 完成后回调 `ReadbackCallback`，上层持有 `ImageBufferPtr` 即可。

---

## 4. 内存池与 utils 层对象管理

> 重点：**所有回读中产生的 buffer，不再通过原始指针传递，而是统一用 `ImageBufferPtr`。回收策略交给对象池 + shared_ptr。**

### 4.1 `ImageBuffer` 抽象（utils）

**在 `third_party/LREngine/include/lrengine/utils/ImageBuffer.h`（示意）定义：**

```cpp
namespace lrengine {
namespace utils {

struct ImageBufferDesc {
    uint32_t    width      = 0;
    uint32_t    height     = 0;
    ImageFormat format     = ImageFormat::RGBA8;
    uint32_t    planeCount = 1;
    std::vector<uint32_t> strides;
};

class ImageBuffer {
public:
    virtual ~ImageBuffer() = default;

    const ImageBufferDesc& GetDesc() const { return mDesc; }

    virtual uint8_t*      GetPlaneData(uint32_t planeIndex) = 0; // CPU 模式
    virtual uint32_t      GetPlaneStride(uint32_t planeIndex) const = 0;
    virtual ResourceHandle GetNativeHandle() const = 0;           // CVPixelBuffer/AHardwareBuffer 等

protected:
    ImageBufferDesc mDesc;
};

using ImageBufferPtr = std::shared_ptr<ImageBuffer>;

} // namespace utils
} // namespace lrengine
```

### 4.2 `ImageBufferPool`（按格式+尺寸分类的对象池）

**在 `third_party/LREngine/src/utils/ImageBufferPool.*` 实现：**

```cpp
class ImageBufferPool {
public:
    explicit ImageBufferPool(size_t maxPoolSizeBytes);

    ImageBufferPtr Acquire(const ImageBufferDesc& desc);
    void           Release(ImageBuffer* buffer);
    void           Trim(size_t targetBytes);

private:
    struct Key {
        uint32_t    width;
        uint32_t    height;
        ImageFormat format;
        uint32_t    planeCount;
        bool operator==(const Key&) const = default;
    };

    struct Bucket {
        std::vector<ImageBuffer*> freeList;
        size_t pooledBytes = 0;
    };

    std::unordered_map<Key, Bucket> mBuckets;
    size_t mTotalBytes;
    size_t mMaxBytes;
};
```

**使用策略：**

- `Acquire`：
  - 若有匹配 desc 的空闲 buffer，直接复用；
  - 否则创建新的 `ImageBuffer`（平台具体类型由 factory 决定）。
- `Release`：
  - 使用自定义 deleter 将 `ImageBuffer*` 放回池中；
  - 若总内存超过上限，直接销毁 buffer（而不入池）。

**生命周期：**

- `ReadbackAsync` 内部从池子 `Acquire` buffer，构造 `ImageBufferPtr`，把它放进 `ReadbackResult`；
- 上层使用完读回数据，释放 `ImageBufferPtr`；
- 自定义 deleter 调用 `ImageBufferPool::Release`，对象回到池中或被销毁。

---

## 5. 执行计划（阶段划分）

下面是一个可以按步骤推进的执行计划。

### 阶段 0：接口冻结与评审

- **输出物**：本方案（你正在看的）+ 补充接口签名草图。
- 确认点：
  - 是否只保留 `LRPlanarTexture` 作为对外纹理类；
  - `UpdateFromImage + GetNativeHandle(planeIndex) + ReadbackAsync` 的接口形态是否满意；
  - 对 CVPixelBuffer/AHardwareBuffer 的零拷贝策略是否符合 Phase2 要求；
  - `readTexture` 旧接口确认完全废弃。

> 当前阶段你只需要确认“接口设计 + 执行顺序”，不涉及任何代码修改。

---

### 阶段 1：core 层类型与接口落地（不改业务调用）

1. 在 `LRTypes.h` 中落地：
   - `ImageFormat` / `ColorSpace` / `ColorRange`
   - `ImagePlaneDesc` / `ImageDataDesc`

2. 在 `LRTexture.h/.cpp` 中：
   - 保留现有接口，仅增加必要的 friend 和辅助方法（如 `IsValid()`），不再扩展对外接口。

3. 在 `LRPlanarTexture.h/.cpp` 中：
   - 为 `UpdateFromImage`、`GetNativeHandle(int planeIndex)` 和 `ReadbackAsync` **增加声明与空实现**（返回 false/默认值），不立即改变现有调用逻辑。
   - 内部保留 `mPlanes` 管理 `LRTexture*` 的结构。

> 阶段 1 结束时，项目应该能正常编译运行，功能与原来一致，仅多了新接口的“骨架”。

---

### 阶段 2：utils 对象池与平台 buffer 类型实现

1. 在 `utils/` 下新增：
   - `ImageBuffer` 抽象类及 `ImageBufferPtr` 定义；
   - `ImageBufferPool` 实现（CPU buffer 版本）。

2. 针对平台：
   - Apple 平台：增加 `MetalCVImageBuffer`（内部持有 `CVPixelBufferRef` + 可选 `CVMetalTextureRef`），实现 `ImageBuffer`。
   - Android 平台：增加 `AndroidHwImageBuffer`（内部持有 `AHardwareBuffer*` 或 GLES PBO + CPU mapped 内存）。

3. 提供一个全局/上下文级的池管理入口（例如挂在 `LRRenderContext` 或 `EngineContext` 上），方便渲染流程调用。

> 阶段 2 重点是 utils，不动 Pipeline / MacCameraApp 的业务逻辑。

---

### 阶段 3：平台 backend 实现 `UpdateFromImage` & `ReadbackAsync`

1. 在 `LRPlanarTexture::UpdateFromImage` 中：
   - 基于 `ImageDataDesc` 拆解 planes；
   - 若尺寸/format 不一致，重建 `mPlanes` 内部的 `LRTexture`；
   - 调用 `LRTexture::UpdateData` 完成 CPU→GPU 更新。
   - 按 `ImageFormat` 映射到对应的 `PlanarFormat` 与 `PixelFormat`。

2. 在各平台 backend 实现 `ReadbackAsync`：

   - **iOS/macOS (Metal)**：
     - 若 `options.externalTarget` 是 `CVPixelBufferRef` 且 `preferZeroCopy`：
       - 使用 CVMetalTexture + blit/compute 实现零拷贝；
       - 用 `ImageBufferPool` 封装该 CVPixelBuffer。
     - 否则：
       - 使用 CPU buffer 类型的 `ImageBuffer`（`CpuImageBuffer`）+ `getBytes` 读回。

   - **Android (GLES/Vulkan)**：
     - 若 `externalTarget` 是 `AHardwareBuffer*`：
       - 用 EGLImage/VkImage 导入后 GPU 写入；
       - 回调后返回包含该句柄的 `ImageBufferPtr`。
     - 否则：
       - 使用 PBO/`glReadPixels` + CPU buffer `ImageBuffer`。

3. 在 `GetNativeHandle(int planeIndex)` 中：
   - 直接返回 `mPlanes[planeIndex]->GetNativeHandle()`；
   - 对单平面纹理，`planeIndex` 必须为 0，其他返回空句柄。

---

### 阶段 4：Pipeline 与业务层切换到新接口

1. 在 Pipeline / IOSMetalContextManager / AndroidCamera 等处：
   - 替换所有直接操作 `LRTexture` 的调用：
     - 改为通过 `LRPlanarTexture::UpdateFromImage` 更新；
     - UI/渲染层通过 `GetNativeHandle(planeIndex)` 拿原生纹理。
2. 移除所有旧的 `readTexture` / 手动 CPU buffer 管理代码，改成使用 `ReadbackAsync + ImageBufferPool`。

3. 针对 Phase2 的具体输出要求（例如导出视频帧 / 截图）：
   - 统一走 `ReadbackAsync` 并拿到 `ImageBufferPtr`；
   - 由上层决定拿 `buffer->GetPlaneData()` 还是通过 `GetNativeHandle` 与平台进一步对接。

---

如果这套**“只保留 LRPlanarTexture + 新 Readback 接口 + utils 对象池管理”**的计划整体符合你预期，我可以按这个执行顺序，先从“阶段 1 + 阶段 2 的骨架代码”开始落地，然后再逐步补齐平台实现与 Pipeline 集成。