/**
 * @file main.mm
 * @brief RenderToTextureMTL示例 - Metal实现离屏渲染和全屏显示
 * 
 * 演示如何使用LREngine的Metal后端实现：
 * - 将三个立方体渲染到离屏纹理（render-to-texture）
 * - 将离屏纹理渲染到全屏显示
 * - FrameBuffer的使用
 * - 多pass渲染流程
 */

#include "lrengine/math/MathFwd.hpp"
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <iostream>
#include <cmath>
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

#include "lrengine/core/LRDefines.h"
#include "lrengine/core/LRTypes.h"
#include "lrengine/core/LRError.h"
#include "lrengine/core/LRBuffer.h"
#include "lrengine/core/LRShader.h"
#include "lrengine/core/LRTexture.h"
#include "lrengine/core/LRRenderContext.h"
#include "lrengine/core/LRPipelineState.h"
#include "lrengine/core/LRFrameBuffer.h"
#include "lrengine/factory/LRDeviceFactory.h"
#include "lrengine/math/Vec3.hpp"
#include "lrengine/math/Vec4.hpp"
#include "lrengine/math/Mat4.hpp"

// stb_image for texture loading
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

using namespace lrengine::render;
using namespace lrengine::math;

// 窗口尺寸
constexpr int WINDOW_WIDTH = 1024;
constexpr int WINDOW_HEIGHT = 768;

// 离屏渲染纹理尺寸
constexpr int OFFSCREEN_WIDTH = 1024;
constexpr int OFFSCREEN_HEIGHT = 768;

// 顶点结构：位置 + 纹理坐标 + 法线
struct Vertex {
    Vec3f position;
    Vec2f texCoord;
    Vec3f normal;
};

// Cube渲染的Uniform数据结构
struct CubeUniforms {
    Mat4f modelMatrix;
    Mat4f viewMatrix;
    Mat4f projectionMatrix;
};

// Quad渲染的Uniform数据结构
struct QuadUniforms {
    Mat4f modelMatrix;
};

// Metal着色器源码 - Cube渲染
const char* cubeShaderSource = R"(
#include <metal_stdlib>
using namespace metal;

// Uniform结构
struct Uniforms {
    float4x4 modelMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
};

// 顶点输入
struct VertexIn {
    float3 position [[attribute(0)]];
    float2 texCoord [[attribute(1)]];
    float3 normal [[attribute(2)]];
};

// 顶点输出/片段输入
struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
    float3 normal;
    float3 worldPos;
};

// 顶点着色器
vertex VertexOut vertexMain(VertexIn in [[stage_in]],
                            constant Uniforms& uniforms [[buffer(1)]]) {
    VertexOut out;
    
    float4x4 mvp = uniforms.projectionMatrix * uniforms.viewMatrix * uniforms.modelMatrix;
    out.position = mvp * float4(in.position, 1.0);
    out.texCoord = in.texCoord;
    out.normal = (uniforms.modelMatrix * float4(in.normal, 0.0)).xyz;
    out.worldPos = (uniforms.modelMatrix * float4(in.position, 1.0)).xyz;
    
    return out;
}

// 片段着色器
fragment float4 fragmentMain(VertexOut in [[stage_in]],
                             texture2d<float> colorTexture [[texture(0)]],
                             sampler texSampler [[sampler(0)]]) {
    // 纹理采样
    float4 texColor = colorTexture.sample(texSampler, in.texCoord);
    
    // 简单的方向光照
    float3 lightDir = normalize(float3(1.0, 1.0, 1.0));
    float3 normal = normalize(in.normal);
    float diffuse = max(dot(normal, lightDir), 0.0);
    
    // 环境光 + 漫反射
    float3 ambient = float3(0.3, 0.3, 0.3);
    float3 lighting = ambient + diffuse * 0.7;
    
    return float4(texColor.rgb * lighting, texColor.a);
}
)";

// Metal着色器源码 - Quad全屏显示
const char* quadShaderSource = R"(
#include <metal_stdlib>
using namespace metal;

// Uniform结构（虽然这里不需要，但保持统一）
struct Uniforms {
    float4x4 modelMatrix;
};

// 顶点输入
struct VertexIn {
    float3 position [[attribute(0)]];
    float2 texCoord [[attribute(1)]];
};

// 顶点输出/片段输入
struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
};

// 顶点着色器 - 全屏Quad
vertex VertexOut vertexMainQuad(VertexIn in [[stage_in]]) {
    VertexOut out;
    out.position = float4(in.position, 1.0);
    out.texCoord = in.texCoord;
    return out;
}

// 片段着色器 - 简单纹理采样
fragment float4 fragmentMainQuad(VertexOut in [[stage_in]],
                                 texture2d<float> colorTexture [[texture(0)]],
                                 sampler texSampler [[sampler(0)]]) {
    return colorTexture.sample(texSampler, in.texCoord);
}
)";

// 立方体顶点数据（36个顶点，6个面）
Vertex cubeVertices[] = {
    // 前面 (Z+)
    {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}, { 0.0f,  0.0f,  1.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f}, { 0.0f,  0.0f,  1.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}, { 0.0f,  0.0f,  1.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}, { 0.0f,  0.0f,  1.0f}},
    {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}, { 0.0f,  0.0f,  1.0f}},
    {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}, { 0.0f,  0.0f,  1.0f}},
    
    // 后面 (Z-)
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, { 0.0f,  0.0f, -1.0f}},
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f}, { 0.0f,  0.0f, -1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f}, { 0.0f,  0.0f, -1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f}, { 0.0f,  0.0f, -1.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f}, { 0.0f,  0.0f, -1.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, { 0.0f,  0.0f, -1.0f}},
    
    // 左面 (X-)
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, {-1.0f,  0.0f,  0.0f}},
    {{-0.5f, -0.5f,  0.5f}, {1.0f, 1.0f}, {-1.0f,  0.0f,  0.0f}},
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}, {-1.0f,  0.0f,  0.0f}},
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}, {-1.0f,  0.0f,  0.0f}},
    {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f}, {-1.0f,  0.0f,  0.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, {-1.0f,  0.0f,  0.0f}},
    
    // 右面 (X+)
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}, { 1.0f,  0.0f,  0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f}, { 1.0f,  0.0f,  0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f}, { 1.0f,  0.0f,  0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f}, { 1.0f,  0.0f,  0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}, { 1.0f,  0.0f,  0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}, { 1.0f,  0.0f,  0.0f}},
    
    // 顶面 (Y+)
    {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}, { 0.0f,  1.0f,  0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}, { 0.0f,  1.0f,  0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f}, { 0.0f,  1.0f,  0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f}, { 0.0f,  1.0f,  0.0f}},
    {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f}, { 0.0f,  1.0f,  0.0f}},
    {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}, { 0.0f,  1.0f,  0.0f}},
    
    // 底面 (Y-)
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, { 0.0f, -1.0f,  0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f}, { 0.0f, -1.0f,  0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}, { 0.0f, -1.0f,  0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}, { 0.0f, -1.0f,  0.0f}},
    {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}, { 0.0f, -1.0f,  0.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, { 0.0f, -1.0f,  0.0f}},
};

// 全屏Quad顶点数据（只需要位置和纹理坐标）
struct QuadVertex {
    Vec3f position;
    Vec2f texCoord;
};

QuadVertex quadVertices[] = {
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},  // 左下
    {{ 1.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},  // 右下
    {{ 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f}},  // 右上
    {{ 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f}},  // 右上
    {{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f}},  // 左上
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},  // 左下
};

// 错误回调
void errorCallback(const ErrorInfo& error)
{
    std::cerr << "[LREngine Error] " << error.message 
              << " (Code: " << static_cast<int>(error.code) << ")"
              << " at " << error.file << ":" << error.line << std::endl;
}

// GLFW错误回调
void glfwErrorCallback(int error, const char* description)
{
    std::cerr << "[GLFW Error] " << error << ": " << description << std::endl;
}

// 生成程序化棋盘纹理
unsigned char* GenerateCheckerboardTexture(int width, int height, int* channels)
{
    *channels = 4;  // RGBA
    unsigned char* data = new unsigned char[width * height * 4];
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int checker = ((x / 32) + (y / 32)) % 2;
            unsigned char color = checker ? 255 : 64;
            
            int idx = (y * width + x) * 4;
            data[idx + 0] = color;      // R
            data[idx + 1] = color;      // G
            data[idx + 2] = color;      // B
            data[idx + 3] = 255;        // A
        }
    }
    
    return data;
}

int main()
{
    @autoreleasepool {
        std::cout << "LREngine RenderToTextureMTL Example" << std::endl;
        std::cout << "=====================================" << std::endl;
        std::cout << "Demonstrating:" << std::endl;
        std::cout << "  - Render-to-Texture (offscreen rendering)" << std::endl;
        std::cout << "  - Three cubes with different transforms" << std::endl;
        std::cout << "  - Fullscreen display of offscreen texture" << std::endl;
        std::cout << "  - Multi-pass rendering pipeline" << std::endl;
        std::cout << std::endl;

        // 设置错误回调
        LRError::SetErrorCallback(errorCallback);
        glfwSetErrorCallback(glfwErrorCallback);

        // 初始化GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return -1;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        // 创建窗口
        GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, 
                                              "LREngine - RenderToTextureMTL", nullptr, nullptr);
        if (!window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return -1;
        }

        // 获取原生窗口句柄
        NSWindow* nsWindow = glfwGetCocoaWindow(window);

        // 创建渲染上下文
        RenderContextDescriptor contextDesc;
        contextDesc.backend = Backend::Metal;
        contextDesc.width = WINDOW_WIDTH;
        contextDesc.height = WINDOW_HEIGHT;
        contextDesc.windowHandle = (__bridge void*)nsWindow;
        contextDesc.vsync = true;

        auto context = LRRenderContext::Create(contextDesc);
        if (!context) {
            std::cerr << "Failed to create Metal render context" << std::endl;
            glfwDestroyWindow(window);
            glfwTerminate();
            return -1;
        }

        std::cout << "Metal render context created successfully" << std::endl;

        // ========== 创建Cube渲染的着色器 ==========
        ShaderDescriptor cubeVsDesc;
        cubeVsDesc.stage = ShaderStage::Vertex;
        cubeVsDesc.language = ShaderLanguage::MSL;
        cubeVsDesc.source = cubeShaderSource;
        cubeVsDesc.entryPoint = "vertexMain";
        cubeVsDesc.debugName = "CubeVS";

        LRShader* cubeVertexShader = context->CreateShader(cubeVsDesc);
        if (!cubeVertexShader || !cubeVertexShader->IsCompiled()) {
            std::cerr << "Failed to compile cube vertex shader" << std::endl;
            return -1;
        }

        ShaderDescriptor cubeFsDesc;
        cubeFsDesc.stage = ShaderStage::Fragment;
        cubeFsDesc.language = ShaderLanguage::MSL;
        cubeFsDesc.source = cubeShaderSource;
        cubeFsDesc.entryPoint = "fragmentMain";
        cubeFsDesc.debugName = "CubeFS";

        LRShader* cubeFragmentShader = context->CreateShader(cubeFsDesc);
        if (!cubeFragmentShader || !cubeFragmentShader->IsCompiled()) {
            std::cerr << "Failed to compile cube fragment shader" << std::endl;
            return -1;
        }

        LRShaderProgram* cubeShaderProgram = context->CreateShaderProgram(cubeVertexShader, cubeFragmentShader);
        if (!cubeShaderProgram || !cubeShaderProgram->IsLinked()) {
            std::cerr << "Failed to link cube shader program" << std::endl;
            return -1;
        }

        std::cout << "Cube shaders compiled and linked successfully" << std::endl;

        // ========== 创建Quad渲染的着色器 ==========
        ShaderDescriptor quadVsDesc;
        quadVsDesc.stage = ShaderStage::Vertex;
        quadVsDesc.language = ShaderLanguage::MSL;
        quadVsDesc.source = quadShaderSource;
        quadVsDesc.entryPoint = "vertexMainQuad";
        quadVsDesc.debugName = "QuadVS";

        LRShader* quadVertexShader = context->CreateShader(quadVsDesc);
        if (!quadVertexShader || !quadVertexShader->IsCompiled()) {
            std::cerr << "Failed to compile quad vertex shader" << std::endl;
            return -1;
        }

        ShaderDescriptor quadFsDesc;
        quadFsDesc.stage = ShaderStage::Fragment;
        quadFsDesc.language = ShaderLanguage::MSL;
        quadFsDesc.source = quadShaderSource;
        quadFsDesc.entryPoint = "fragmentMainQuad";
        quadFsDesc.debugName = "QuadFS";

        LRShader* quadFragmentShader = context->CreateShader(quadFsDesc);
        if (!quadFragmentShader || !quadFragmentShader->IsCompiled()) {
            std::cerr << "Failed to compile quad fragment shader" << std::endl;
            return -1;
        }

        LRShaderProgram* quadShaderProgram = context->CreateShaderProgram(quadVertexShader, quadFragmentShader);
        if (!quadShaderProgram || !quadShaderProgram->IsLinked()) {
            std::cerr << "Failed to link quad shader program" << std::endl;
            return -1;
        }

        std::cout << "Quad shaders compiled and linked successfully" << std::endl;

        // ========== 创建Cube顶点缓冲区 ==========
        BufferDescriptor cubeVbDesc;
        cubeVbDesc.size = sizeof(cubeVertices);
        cubeVbDesc.usage = BufferUsage::Static;
        cubeVbDesc.type = BufferType::Vertex;
        cubeVbDesc.data = cubeVertices;
        cubeVbDesc.stride = sizeof(Vertex);
        cubeVbDesc.debugName = "CubeVB";

        LRVertexBuffer* cubeVertexBuffer = context->CreateVertexBuffer(cubeVbDesc);
        if (!cubeVertexBuffer) {
            std::cerr << "Failed to create cube vertex buffer" << std::endl;
            return -1;
        }

        // 设置Cube顶点布局
        VertexLayoutDescriptor cubeLayout;
        cubeLayout.stride = sizeof(Vertex);
        
        VertexAttribute posAttr;
        posAttr.location = 0;
        posAttr.format = VertexFormat::Float3;
        posAttr.offset = offsetof(Vertex, position);
        cubeLayout.attributes.push_back(posAttr);
        
        VertexAttribute texAttr;
        texAttr.location = 1;
        texAttr.format = VertexFormat::Float2;
        texAttr.offset = offsetof(Vertex, texCoord);
        cubeLayout.attributes.push_back(texAttr);
        
        VertexAttribute normalAttr;
        normalAttr.location = 2;
        normalAttr.format = VertexFormat::Float3;
        normalAttr.offset = offsetof(Vertex, normal);
        cubeLayout.attributes.push_back(normalAttr);

        cubeVertexBuffer->SetVertexLayout(cubeLayout);

        // ========== 创建Quad顶点缓冲区 ==========
        BufferDescriptor quadVbDesc;
        quadVbDesc.size = sizeof(quadVertices);
        quadVbDesc.usage = BufferUsage::Static;
        quadVbDesc.type = BufferType::Vertex;
        quadVbDesc.data = quadVertices;
        quadVbDesc.stride = sizeof(QuadVertex);
        quadVbDesc.debugName = "QuadVB";

        LRVertexBuffer* quadVertexBuffer = context->CreateVertexBuffer(quadVbDesc);
        if (!quadVertexBuffer) {
            std::cerr << "Failed to create quad vertex buffer" << std::endl;
            return -1;
        }

        // 设置Quad顶点布局
        VertexLayoutDescriptor quadLayout;
        quadLayout.stride = sizeof(QuadVertex);
        
        VertexAttribute quadPosAttr;
        quadPosAttr.location = 0;
        quadPosAttr.format = VertexFormat::Float3;
        quadPosAttr.offset = offsetof(QuadVertex, position);
        quadLayout.attributes.push_back(quadPosAttr);
        
        VertexAttribute quadTexAttr;
        quadTexAttr.location = 1;
        quadTexAttr.format = VertexFormat::Float2;
        quadTexAttr.offset = offsetof(QuadVertex, texCoord);
        quadLayout.attributes.push_back(quadTexAttr);

        quadVertexBuffer->SetVertexLayout(quadLayout);

        std::cout << "Vertex buffers created" << std::endl;

        // ========== 加载立方体纹理 ==========
        const char* texturePath = "../examples/resources/MG_1822.PNG";
        int texWidth, texHeight, texChannels;
        
        std::cout << "Loading texture from: " << texturePath << std::endl;
        unsigned char* texData = stbi_load(texturePath, &texWidth, &texHeight, &texChannels, 4);
        
        if (!texData) {
            std::cerr << "Failed to load texture, using checkerboard..." << std::endl;
            texWidth = 256;
            texHeight = 256;
            texData = GenerateCheckerboardTexture(texWidth, texHeight, &texChannels);
        } else {
            std::cout << "Texture loaded: " << texWidth << "x" << texHeight << std::endl;
            texChannels = 4;
        }

        TextureDescriptor cubeTexDesc;
        cubeTexDesc.type = TextureType::Texture2D;
        cubeTexDesc.width = texWidth;
        cubeTexDesc.height = texHeight;
        cubeTexDesc.depth = 1;
        cubeTexDesc.format = PixelFormat::RGBA8;
        cubeTexDesc.mipLevels = 1;
        cubeTexDesc.data = texData;
        cubeTexDesc.generateMipmaps = false;
        cubeTexDesc.debugName = "CubeTexture";
        cubeTexDesc.sampler.minFilter = FilterMode::Linear;
        cubeTexDesc.sampler.magFilter = FilterMode::Linear;
        cubeTexDesc.sampler.wrapU = WrapMode::Repeat;
        cubeTexDesc.sampler.wrapV = WrapMode::Repeat;

        LRTexture* cubeTexture = context->CreateTexture(cubeTexDesc);
        
        if (texData) {
            stbi_image_free(texData);
        }

        if (!cubeTexture) {
            std::cerr << "Failed to create cube texture" << std::endl;
            return -1;
        }

        std::cout << "Cube texture created successfully" << std::endl;

        // ========== 创建离屏渲染目标（FrameBuffer + Texture） ==========
        // 创建离屏颜色纹理
        TextureDescriptor offscreenColorDesc;
        offscreenColorDesc.type = TextureType::Texture2D;
        offscreenColorDesc.width = OFFSCREEN_WIDTH;
        offscreenColorDesc.height = OFFSCREEN_HEIGHT;
        offscreenColorDesc.depth = 1;
        offscreenColorDesc.format = PixelFormat::RGBA8;
        offscreenColorDesc.mipLevels = 1;
        offscreenColorDesc.data = nullptr;  // 不需要初始数据
        offscreenColorDesc.generateMipmaps = false;
        offscreenColorDesc.debugName = "OffscreenColor";
        offscreenColorDesc.sampler.minFilter = FilterMode::Linear;
        offscreenColorDesc.sampler.magFilter = FilterMode::Linear;
        offscreenColorDesc.sampler.wrapU = WrapMode::ClampToEdge;
        offscreenColorDesc.sampler.wrapV = WrapMode::ClampToEdge;

        LRTexture* offscreenColorTexture = context->CreateTexture(offscreenColorDesc);
        if (!offscreenColorTexture) {
            std::cerr << "Failed to create offscreen color texture" << std::endl;
            return -1;
        }

        // 创建离屏深度纹理
        TextureDescriptor offscreenDepthDesc;
        offscreenDepthDesc.type = TextureType::Texture2D;
        offscreenDepthDesc.width = OFFSCREEN_WIDTH;
        offscreenDepthDesc.height = OFFSCREEN_HEIGHT;
        offscreenDepthDesc.depth = 1;
        offscreenDepthDesc.format = PixelFormat::Depth32F;
        offscreenDepthDesc.mipLevels = 1;
        offscreenDepthDesc.data = nullptr;
        offscreenDepthDesc.generateMipmaps = false;
        offscreenDepthDesc.debugName = "OffscreenDepth";

        LRTexture* offscreenDepthTexture = context->CreateTexture(offscreenDepthDesc);
        if (!offscreenDepthTexture) {
            std::cerr << "Failed to create offscreen depth texture" << std::endl;
            return -1;
        }

        // 创建FrameBuffer
        FrameBufferDescriptor fbDesc;
        fbDesc.width = OFFSCREEN_WIDTH;
        fbDesc.height = OFFSCREEN_HEIGHT;
        fbDesc.hasDepthStencil = true;
        fbDesc.debugName = "OffscreenFB";
        
        // 配置颜色附件
        ColorAttachmentDescriptor colorAttachment;
        colorAttachment.format = PixelFormat::RGBA8;
        colorAttachment.loadOp = LoadOp::Clear;
        colorAttachment.storeOp = StoreOp::Store;
        colorAttachment.clearColor[0] = 0.1f;
        colorAttachment.clearColor[1] = 0.1f;
        colorAttachment.clearColor[2] = 0.15f;
        colorAttachment.clearColor[3] = 1.0f;
        fbDesc.colorAttachments.push_back(colorAttachment);
        
        // 配置深度模板附件
        fbDesc.depthStencilAttachment.format = PixelFormat::Depth32F;
        fbDesc.depthStencilAttachment.depthLoadOp = LoadOp::Clear;
        fbDesc.depthStencilAttachment.depthStoreOp = StoreOp::Store;
        fbDesc.depthStencilAttachment.clearDepth = 1.0f;

        LRFrameBuffer* offscreenFrameBuffer = context->CreateFrameBuffer(fbDesc);
        if (!offscreenFrameBuffer) {
            std::cerr << "Failed to create offscreen framebuffer" << std::endl;
            return -1;
        }
        
        // 附加纹理到FrameBuffer
        offscreenFrameBuffer->AttachColorTexture(offscreenColorTexture, 0);
        offscreenFrameBuffer->AttachDepthTexture(offscreenDepthTexture);
        
        // 调试：打印纹理地址
        std::cout << "\nTexture Debug Info:" << std::endl;
        std::cout << "  offscreenColorTexture (LRTexture):  " << offscreenColorTexture << std::endl;
        std::cout << "  NativeHandle().ptr:                 " << offscreenColorTexture->GetNativeHandle().ptr << std::endl;
        std::cout << std::endl;
        
        if (!offscreenFrameBuffer->IsComplete()) {
            std::cerr << "Offscreen framebuffer is not complete" << std::endl;
            return -1;
        }

        std::cout << "Offscreen framebuffer created successfully" << std::endl;

        // ========== 创建管线状态 ==========
        // Cube渲染管线
        PipelineStateDescriptor cubePipelineDesc;
        cubePipelineDesc.vertexShader = cubeVertexShader;
        cubePipelineDesc.fragmentShader = cubeFragmentShader;
        cubePipelineDesc.vertexLayout = cubeLayout;
        cubePipelineDesc.primitiveType = PrimitiveType::Triangles;
        cubePipelineDesc.depthStencilState.depthTestEnabled = true;
        cubePipelineDesc.depthStencilState.depthWriteEnabled = true;
        cubePipelineDesc.depthStencilState.depthCompareFunc = CompareFunc::Less;
        cubePipelineDesc.rasterizerState.cullMode = CullMode::Back;
        cubePipelineDesc.rasterizerState.frontFace = FrontFace::CCW;
        cubePipelineDesc.debugName = "CubePipeline";

        LRPipelineState* cubePipelineState = context->CreatePipelineState(cubePipelineDesc);
        if (!cubePipelineState) {
            std::cerr << "Failed to create cube pipeline state" << std::endl;
            return -1;
        }

        // Quad渲染管线（全屏显示，不需要深度测试）
        PipelineStateDescriptor quadPipelineDesc;
        quadPipelineDesc.vertexShader = quadVertexShader;
        quadPipelineDesc.fragmentShader = quadFragmentShader;
        quadPipelineDesc.vertexLayout = quadLayout;
        quadPipelineDesc.primitiveType = PrimitiveType::Triangles;
        quadPipelineDesc.depthStencilState.depthTestEnabled = false;
        quadPipelineDesc.depthStencilState.depthWriteEnabled = false;
        quadPipelineDesc.rasterizerState.cullMode = CullMode::None;
        quadPipelineDesc.debugName = "QuadPipeline";

        LRPipelineState* quadPipelineState = context->CreatePipelineState(quadPipelineDesc);
        if (!quadPipelineState) {
            std::cerr << "Failed to create quad pipeline state" << std::endl;
            return -1;
        }

        std::cout << "Pipeline states created successfully" << std::endl;

        // ========== 创建Uniform缓冲区 ==========
        BufferDescriptor cubeUniformDesc;
        cubeUniformDesc.size = sizeof(CubeUniforms);
        cubeUniformDesc.usage = BufferUsage::Dynamic;
        cubeUniformDesc.type = BufferType::Uniform;
        cubeUniformDesc.debugName = "CubeUniformBuffer";

        LRUniformBuffer* cubeUniformBuffer = context->CreateUniformBuffer(cubeUniformDesc);
        if (!cubeUniformBuffer) {
            std::cerr << "Failed to create cube uniform buffer" << std::endl;
            return -1;
        }

        // 主循环
        std::cout << "\nRendering three cubes to texture, then displaying fullscreen..." << std::endl;
        std::cout << "Press ESC to exit" << std::endl;

        float rotationAngle = 0.0f;

        while (!glfwWindowShouldClose(window)) {
            // 处理输入
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, true);
            }

            // 更新旋转角度
            rotationAngle += 0.01f;

            // 开始帧
            context->BeginFrame();

            // ========== Pass 1: 测试 - 清除为纯红色 ==========
            // 注意：在Metal后端，Clear需要在BeginRenderPass之前调用以设置清除值
            context->Clear(ClearFlag_Color | ClearFlag_Depth, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0);
            context->BeginRenderPass(offscreenFrameBuffer);
            
            cubeShaderProgram->Use();
            context->SetPipelineState(cubePipelineState);
            context->SetVertexBuffer(cubeVertexBuffer, 0);
            context->SetTexture(cubeTexture, 0);

            // 共享的视图和投影矩阵
            Vec3f eye(0.0f, 0.0f, 20.0f);
            Vec3f center(0.0f, 0.0f, 0.0f);
            Vec3f up(0.0f, 1.0f, 0.0f);
            Mat4f viewMatrix = Mat4f::lookAt(eye, center, up).transpose();
            
            float aspect = static_cast<float>(OFFSCREEN_WIDTH) / static_cast<float>(OFFSCREEN_HEIGHT);
            Mat4f projectionMatrix = Mat4f::perspective(45.0f * 3.14159f / 180.0f, aspect, 0.1f, 100.0f);

            // 绘制第一个立方体（左侧，绕Y轴旋转）
            {
                CubeUniforms uniforms;
                Mat4f translation = Mat4f::translate(Vec3f(-2.5f, 0.0f, 0.0f));
                Mat4f rotation = Mat4f::rotateY(rotationAngle);
                uniforms.modelMatrix = (translation * rotation).transpose();
                uniforms.viewMatrix = viewMatrix;
                uniforms.projectionMatrix = projectionMatrix;
                
                cubeUniformBuffer->UpdateData(&uniforms, sizeof(CubeUniforms), 0);
                context->SetUniformBuffer(cubeUniformBuffer, 1);
                context->Draw(0, 36);
            }

            // 绘制第二个立方体（中间，绕X轴旋转，稍微放大）
            {
                CubeUniforms uniforms;
                Mat4f scale = Mat4f::scale(Vec3f(1.3f, 1.3f, 1.3f));
                Mat4f rotation = Mat4f::rotateX(rotationAngle * 1.5f);
                uniforms.modelMatrix = (scale * rotation).transpose();
                uniforms.viewMatrix = viewMatrix;
                uniforms.projectionMatrix = projectionMatrix;
                
                cubeUniformBuffer->UpdateData(&uniforms, sizeof(CubeUniforms), 0);
                context->SetUniformBuffer(cubeUniformBuffer, 1);
                context->Draw(0, 36);
            }

            // 绘制第三个立方体（右侧，绕Z轴旋转，缩小）
            {
                CubeUniforms uniforms;
                Mat4f translation = Mat4f::translate(Vec3f(2.5f, 0.0f, 0.0f));
                Mat4f scale = Mat4f::scale(Vec3f(0.7f, 0.7f, 0.7f));
                Mat4f rotation = Mat4f::rotateZ(rotationAngle * 2.0f);
                uniforms.modelMatrix = (translation * scale * rotation).transpose();
                uniforms.viewMatrix = viewMatrix;
                uniforms.projectionMatrix = projectionMatrix;
                
                cubeUniformBuffer->UpdateData(&uniforms, sizeof(CubeUniforms), 0);
                context->SetUniformBuffer(cubeUniformBuffer, 1);
                context->Draw(0, 36);
            }
            
            context->EndRenderPass();
            
            // ========== Pass 2: 将离屏纹理渲染到屏幕 ==========
            context->Clear(ClearFlag_Color, 0.0f, 0.0f, 0.5f, 1.0f, 1.0f, 0);
            context->BeginRenderPass(nullptr);  // 绑定到默认帧缓冲（屏幕）
            
            quadShaderProgram->Use();
            context->SetPipelineState(quadPipelineState);
            context->SetVertexBuffer(quadVertexBuffer, 0);
            context->SetTexture(offscreenColorTexture, 0);  // 使用离屏渲染的结果
            context->Draw(0, 6);
            
            context->EndRenderPass();

            // 结束帧并呈现
            context->EndFrame();

            glfwPollEvents();
        }

        // 清理资源
        std::cout << "\nCleaning up..." << std::endl;

        delete cubeUniformBuffer;
        delete offscreenFrameBuffer;
        delete offscreenDepthTexture;
        delete offscreenColorTexture;
        delete cubeTexture;
        delete quadPipelineState;
        delete cubePipelineState;
        delete quadVertexBuffer;
        delete cubeVertexBuffer;
        delete quadShaderProgram;
        delete quadFragmentShader;
        delete quadVertexShader;
        delete cubeShaderProgram;
        delete cubeFragmentShader;
        delete cubeVertexShader;
        LRRenderContext::Destroy(context);

        glfwDestroyWindow(window);
        glfwTerminate();

        std::cout << "Done!" << std::endl;
        return 0;
    }
}
