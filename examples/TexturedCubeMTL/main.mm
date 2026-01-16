/**
 * @file main.mm
 * @brief TexturedCubeMTL示例 - Metal渲染带纹理的3D立方体
 * 
 * 演示如何使用LREngine的Metal后端实现：
 * - 3D立方体几何体的创建
 * - 纹理加载和采样
 * - 3D变换矩阵（MVP）
 * - 旋转动画
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
#include "lrengine/factory/LRDeviceFactory.h"
#include "lrengine/math/Vec3.hpp"
#include "lrengine/math/Vec4.hpp"
#include "lrengine/math/Mat4.hpp"

// stb_image for texture loading
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

using namespace lrengine::render;
using namespace hyengine::math;

// 窗口尺寸
constexpr int WINDOW_WIDTH = 1024;
constexpr int WINDOW_HEIGHT = 768;

// 顶点结构：位置 + 纹理坐标 + 法线
struct Vertex {
    Vec3 position;
    Vec2 texCoord;
    Vec3 normal;
};

// Uniform数据结构
struct Uniforms {
    Mat4 modelMatrix;
    Mat4 viewMatrix;
    Mat4 projectionMatrix;
};

// Metal着色器源码 (MSL)
const char* metalShaderSource = R"(
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
        std::cout << "LREngine TexturedCubeMTL Example" << std::endl;
        std::cout << "=================================" << std::endl;
        std::cout << "Demonstrating:" << std::endl;
        std::cout << "  - 3D textured cube rendering with Metal" << std::endl;
        std::cout << "  - MVP matrices and 3D transformations" << std::endl;
        std::cout << "  - Texture loading and sampling" << std::endl;
        std::cout << "  - Rotation animation" << std::endl;
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
                                              "LREngine - TexturedCubeMTL", nullptr, nullptr);
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

        // 创建顶点着色器
        ShaderDescriptor vsDesc;
        vsDesc.stage = ShaderStage::Vertex;
        vsDesc.language = ShaderLanguage::MSL;
        vsDesc.source = metalShaderSource;
        vsDesc.entryPoint = "vertexMain";
        vsDesc.debugName = "CubeVS";

        LRShader* vertexShader = context->CreateShader(vsDesc);
        if (!vertexShader || !vertexShader->IsCompiled()) {
            std::cerr << "Failed to compile vertex shader" << std::endl;
            if (vertexShader) {
                std::cerr << "Error: " << vertexShader->GetCompileError() << std::endl;
            }
            glfwDestroyWindow(window);
            glfwTerminate();
            return -1;
        }

        // 创建片段着色器
        ShaderDescriptor fsDesc;
        fsDesc.stage = ShaderStage::Fragment;
        fsDesc.language = ShaderLanguage::MSL;
        fsDesc.source = metalShaderSource;
        fsDesc.entryPoint = "fragmentMain";
        fsDesc.debugName = "CubeFS";

        LRShader* fragmentShader = context->CreateShader(fsDesc);
        if (!fragmentShader || !fragmentShader->IsCompiled()) {
            std::cerr << "Failed to compile fragment shader" << std::endl;
            glfwDestroyWindow(window);
            glfwTerminate();
            return -1;
        }

        // 创建着色器程序
        LRShaderProgram* shaderProgram = context->CreateShaderProgram(vertexShader, fragmentShader);
        if (!shaderProgram || !shaderProgram->IsLinked()) {
            std::cerr << "Failed to link shader program" << std::endl;
            glfwDestroyWindow(window);
            glfwTerminate();
            return -1;
        }

        std::cout << "Shaders compiled and linked successfully" << std::endl;

        // 创建顶点缓冲区
        BufferDescriptor vbDesc;
        vbDesc.size = sizeof(cubeVertices);
        vbDesc.usage = BufferUsage::Static;
        vbDesc.type = BufferType::Vertex;
        vbDesc.data = cubeVertices;
        vbDesc.stride = sizeof(Vertex);
        vbDesc.debugName = "CubeVB";

        LRVertexBuffer* vertexBuffer = context->CreateVertexBuffer(vbDesc);
        if (!vertexBuffer) {
            std::cerr << "Failed to create vertex buffer" << std::endl;
            glfwDestroyWindow(window);
            glfwTerminate();
            return -1;
        }

        // 设置顶点布局
        VertexLayoutDescriptor layout;
        layout.stride = sizeof(Vertex);
        
        // 位置
        VertexAttribute posAttr;
        posAttr.location = 0;
        posAttr.format = VertexFormat::Float3;
        posAttr.offset = offsetof(Vertex, position);
        layout.attributes.push_back(posAttr);
        
        // 纹理坐标
        VertexAttribute texAttr;
        texAttr.location = 1;
        texAttr.format = VertexFormat::Float2;
        texAttr.offset = offsetof(Vertex, texCoord);
        layout.attributes.push_back(texAttr);
        
        // 法线
        VertexAttribute normalAttr;
        normalAttr.location = 2;
        normalAttr.format = VertexFormat::Float3;
        normalAttr.offset = offsetof(Vertex, normal);
        layout.attributes.push_back(normalAttr);

        vertexBuffer->SetVertexLayout(layout);

        std::cout << "Vertex buffer created" << std::endl;

        // 加载纹理图片
        const char* texturePath = "../examples/resources/MG_1822.PNG";
        int texWidth, texHeight, texChannels;
        
        std::cout << "Loading texture from: " << texturePath << std::endl;
        unsigned char* texData = stbi_load(texturePath, &texWidth, &texHeight, &texChannels, 4);  // 强制4通道RGBA
        
        if (!texData) {
            std::cerr << "Failed to load texture: " << texturePath << std::endl;
            std::cerr << "stbi_failure_reason: " << stbi_failure_reason() << std::endl;
            std::cout << "Falling back to procedural checkerboard texture..." << std::endl;
            
            // 回退到程序化纹理
            texWidth = 256;
            texHeight = 256;
            texData = GenerateCheckerboardTexture(texWidth, texHeight, &texChannels);
        } else {
            std::cout << "Texture loaded successfully: " << texWidth << "x" << texHeight 
                      << " (" << texChannels << " channels)" << std::endl;
            texChannels = 4;  // 我们强制加载为4通道
        }

        TextureDescriptor texDesc;
        texDesc.type = TextureType::Texture2D;
        texDesc.width = texWidth;
        texDesc.height = texHeight;
        texDesc.depth = 1;
        texDesc.format = PixelFormat::RGBA8;
        texDesc.mipLevels = 1;
        texDesc.data = texData;
        texDesc.generateMipmaps = false;
        texDesc.debugName = "CubeTexture";
        
        // 配置采样器
        texDesc.sampler.minFilter = FilterMode::Linear;
        texDesc.sampler.magFilter = FilterMode::Linear;
        texDesc.sampler.mipFilter = FilterMode::Linear;
        texDesc.sampler.wrapU = WrapMode::Repeat;
        texDesc.sampler.wrapV = WrapMode::Repeat;

        LRTexture* texture = context->CreateTexture(texDesc);
        
        if (texData) {
            stbi_image_free(texData);  // 使用stbi_image_free而不是delete[]
        }

        if (!texture) {
            std::cerr << "Failed to create texture" << std::endl;
            glfwDestroyWindow(window);
            glfwTerminate();
            return -1;
        }

        std::cout << "Texture created successfully" << std::endl;

        // 创建管线状态
        PipelineStateDescriptor pipelineDesc;
        pipelineDesc.vertexShader = vertexShader;
        pipelineDesc.fragmentShader = fragmentShader;
        pipelineDesc.vertexLayout = layout;
        pipelineDesc.primitiveType = PrimitiveType::Triangles;
        
        // 启用深度测试
        pipelineDesc.depthStencilState.depthTestEnabled = true;
        pipelineDesc.depthStencilState.depthWriteEnabled = true;
        pipelineDesc.depthStencilState.depthCompareFunc = CompareFunc::Less;
        
        // 背面剔除（正面朝向逆时针）
        pipelineDesc.rasterizerState.cullMode = CullMode::Front;
        pipelineDesc.rasterizerState.frontFace = FrontFace::CCW;
        
        pipelineDesc.debugName = "CubePipeline";

        std::cout << "Pipeline config: depthTest=ON, cullMode=Back, frontFace=CCW" << std::endl;

        LRPipelineState* pipelineState = context->CreatePipelineState(pipelineDesc);
        if (!pipelineState) {
            std::cerr << "Failed to create pipeline state" << std::endl;
            glfwDestroyWindow(window);
            glfwTerminate();
            return -1;
        }

        std::cout << "Pipeline state created successfully" << std::endl;

        // 创建Uniform缓冲区
        BufferDescriptor uniformDesc;
        uniformDesc.size = sizeof(Uniforms);
        uniformDesc.usage = BufferUsage::Dynamic;
        uniformDesc.type = BufferType::Uniform;
        uniformDesc.debugName = "UniformBuffer";

        LRUniformBuffer* uniformBuffer = context->CreateUniformBuffer(uniformDesc);
        if (!uniformBuffer) {
            std::cerr << "Failed to create uniform buffer" << std::endl;
            glfwDestroyWindow(window);
            glfwTerminate();
            return -1;
        }

        // 主循环
        std::cout << "\nRendering textured cube with Metal..." << std::endl;
        std::cout << "Press ESC to exit" << std::endl;

        float rotationAngle = 0.0f;

        while (!glfwWindowShouldClose(window)) {
            // 处理输入
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, true);
            }

            // 更新旋转角度
            rotationAngle += 0.01f;

            // 计算MVP矩阵
            Uniforms uniforms;
            
            // 模型矩阵 - 旋转立方体
            uniforms.modelMatrix = Mat4::rotateY(rotationAngle) * Mat4::rotateX(rotationAngle * 0.5f);
            
            // 视图矩阵 - 相机位置
            Vec3 eye(0.0f, 0.0f, 1.0f);  // 测试：相机远离立方
            Vec3 center(0.0f, 0.0f, 0.0f);
            Vec3 up(0.0f, 1.0f, 0.0f);
            uniforms.viewMatrix = Mat4::lookAt(eye, center, up);
            
            // 投影矩阵 - 透视投影（far改回100，避免深度精度问题）
            float aspect = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);
            uniforms.projectionMatrix = Mat4::perspective(45.0f * 3.14159f / 180.0f, aspect, 0.1f, 100.0f);
            
            // 调试：打印相机位置（仅第一帧）
            static bool firstFrameDebug = true;
            if (firstFrameDebug) {
                std::cout << "\n=== Camera Debug Info ===" << std::endl;
                std::cout << "Eye position: (" << eye.x << ", " << eye.y << ", " << eye.z << ")" << std::endl;
                std::cout << "Look at: (" << center.x << ", " << center.y << ", " << center.z << ")" << std::endl;
                std::cout << "Near plane: 0.1, Far plane: 100.0" << std::endl;
                std::cout << "Cube should be visible (distance: " << eye.z << ")" << std::endl;
                firstFrameDebug = false;
            }

            // 更新Uniform缓冲区
            uniformBuffer->UpdateData(&uniforms, sizeof(Uniforms), 0);

            // 开始帧
            context->BeginFrame();

            // 清除屏幕
            context->Clear(ClearFlag_Color | ClearFlag_Depth, 0.2f, 0.3f, 0.4f, 1.0f, 1.0f, 0);

            // 使用着色器程序
            shaderProgram->Use();

            // 设置管线状态
            context->SetPipelineState(pipelineState);

            // 绑定资源
            context->SetVertexBuffer(vertexBuffer, 0);
            context->SetUniformBuffer(uniformBuffer, 1);
            context->SetTexture(texture, 0);

            // 绘制立方体
            context->Draw(0, 36);

            // 结束帧并呈现
            context->EndFrame();

            glfwPollEvents();
        }

        // 清理资源
        std::cout << "\nCleaning up..." << std::endl;

        delete uniformBuffer;
        delete pipelineState;
        delete texture;
        delete vertexBuffer;
        delete shaderProgram;
        delete fragmentShader;
        delete vertexShader;
        LRRenderContext::Destroy(context);

        glfwDestroyWindow(window);
        glfwTerminate();

        std::cout << "Done!" << std::endl;
        return 0;
    }
}
