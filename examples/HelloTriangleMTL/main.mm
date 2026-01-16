/**
 * @file main.mm
 * @brief HelloTriangleMTL示例 - Metal渲染演示
 * 
 * 演示如何使用LREngine的Metal后端渲染一个简单的彩色三角形
 * 展示Metal特有的功能：命令队列、命令缓冲区、渲染管道等
 */

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <iostream>
#include <GLFW/glfw3.h>

// GLFW需要定义这个宏来支持Metal
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

#include "lrengine/core/LRDefines.h"
#include "lrengine/core/LRTypes.h"
#include "lrengine/core/LRError.h"
#include "lrengine/core/LRBuffer.h"
#include "lrengine/core/LRShader.h"
#include "lrengine/core/LRPipelineState.h"
#include "lrengine/core/LRRenderContext.h"
#include "lrengine/factory/LRDeviceFactory.h"

using namespace lrengine::render;

// 窗口尺寸
constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;

// Metal着色器源码 (MSL - Metal Shading Language)
const char* metalShaderSource = R"(
#include <metal_stdlib>
using namespace metal;

// 顶点输入结构
struct VertexIn {
    float3 position [[attribute(0)]];
    float3 color [[attribute(1)]];
};

// 顶点输出/片段输入结构
struct VertexOut {
    float4 position [[position]];
    float3 color;
};

// 顶点着色器
vertex VertexOut vertexMain(VertexIn in [[stage_in]]) {
    VertexOut out;
    out.position = float4(in.position, 1.0);
    out.color = in.color;
    return out;
}

// 片段着色器
fragment float4 fragmentMain(VertexOut in [[stage_in]]) {
    return float4(in.color, 1.0);
}
)";

// 三角形顶点数据（位置 + 颜色）
float vertices[] = {
    // 位置              // 颜色
    -0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  // 左下 - 红色
     0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,  // 右下 - 绿色
     0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f   // 顶部 - 蓝色
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

// 窗口大小改变回调
void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    (void)window;
    (void)width;
    (void)height;
    // Metal会自动处理drawable大小变化
}

int main()
{
    @autoreleasepool {
        std::cout << "LREngine HelloTriangleMTL Example" << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << "Demonstrating Metal backend features:" << std::endl;
        std::cout << "  - Metal command queue and command buffer" << std::endl;
        std::cout << "  - Metal render pipeline state" << std::endl;
        std::cout << "  - Metal Shading Language (MSL)" << std::endl;
        std::cout << std::endl;

        // 设置错误回调
        LRError::SetErrorCallback(errorCallback);
        glfwSetErrorCallback(glfwErrorCallback);

        // 初始化GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return -1;
        }

        // 对于Metal，我们不需要OpenGL上下文
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        // 创建窗口
        GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, 
                                              "LREngine - HelloTriangleMTL", nullptr, nullptr);
        if (!window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return -1;
        }

        glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

        // 获取原生窗口句柄
        NSWindow* nsWindow = glfwGetCocoaWindow(window);

        // 检查Metal是否可用
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) {
            std::cerr << "Metal is not supported on this device" << std::endl;
            glfwDestroyWindow(window);
            glfwTerminate();
            return -1;
        }

        std::cout << "Metal Device: " << [[device name] UTF8String] << std::endl;

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
        vsDesc.debugName = "TriangleVS";

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

        std::cout << "Metal vertex shader compiled successfully" << std::endl;

        // 创建片段着色器
        ShaderDescriptor fsDesc;
        fsDesc.stage = ShaderStage::Fragment;
        fsDesc.language = ShaderLanguage::MSL;
        fsDesc.source = metalShaderSource;
        fsDesc.entryPoint = "fragmentMain";
        fsDesc.debugName = "TriangleFS";

        LRShader* fragmentShader = context->CreateShader(fsDesc);
        if (!fragmentShader || !fragmentShader->IsCompiled()) {
            std::cerr << "Failed to compile fragment shader" << std::endl;
            if (fragmentShader) {
                std::cerr << "Error: " << fragmentShader->GetCompileError() << std::endl;
            }
            glfwDestroyWindow(window);
            glfwTerminate();
            return -1;
        }

        std::cout << "Metal fragment shader compiled successfully" << std::endl;

        // 创建着色器程序
        LRShaderProgram* shaderProgram = context->CreateShaderProgram(vertexShader, fragmentShader);
        if (!shaderProgram) {
            std::cerr << "Failed to create shader program" << std::endl;
            glfwDestroyWindow(window);
            glfwTerminate();
            return -1;
        }

        if (!shaderProgram->IsLinked()) {
            std::cerr << "Failed to link shader program" << std::endl;
            std::cerr << "Error: " << shaderProgram->GetLinkError() << std::endl;
            glfwDestroyWindow(window);
            glfwTerminate();
            return -1;
        }

        std::cout << "Metal shader program linked successfully" << std::endl;

        // 创建顶点缓冲区
        BufferDescriptor vbDesc;
        vbDesc.size = sizeof(vertices);
        vbDesc.usage = BufferUsage::Static;
        vbDesc.type = BufferType::Vertex;
        vbDesc.data = vertices;
        vbDesc.stride = 6 * sizeof(float);  // 位置(3) + 颜色(3)
        vbDesc.debugName = "TriangleVB";

        LRVertexBuffer* vertexBuffer = context->CreateVertexBuffer(vbDesc);
        if (!vertexBuffer) {
            std::cerr << "Failed to create vertex buffer" << std::endl;
            glfwDestroyWindow(window);
            glfwTerminate();
            return -1;
        }

        // 设置顶点布局
        VertexLayoutDescriptor layout;
        layout.stride = 6 * sizeof(float);
        
        // 位置属性
        VertexAttribute posAttr;
        posAttr.location = 0;
        posAttr.format = VertexFormat::Float3;
        posAttr.offset = 0;
        layout.attributes.push_back(posAttr);
        
        // 颜色属性
        VertexAttribute colorAttr;
        colorAttr.location = 1;
        colorAttr.format = VertexFormat::Float3;
        colorAttr.offset = 3 * sizeof(float);
        layout.attributes.push_back(colorAttr);

        vertexBuffer->SetVertexLayout(layout);

        std::cout << "Metal vertex buffer created successfully" << std::endl;

        // 创建管线状态
        PipelineStateDescriptor pipelineDesc;
        pipelineDesc.vertexShader = vertexShader;
        pipelineDesc.fragmentShader = fragmentShader;
        pipelineDesc.vertexLayout = layout;
        pipelineDesc.primitiveType = PrimitiveType::Triangles;
        pipelineDesc.debugName = "TrianglePipeline";

        LRPipelineState* pipelineState = context->CreatePipelineState(pipelineDesc);
        if (!pipelineState) {
            std::cerr << "Failed to create pipeline state" << std::endl;
            glfwDestroyWindow(window);
            glfwTerminate();
            return -1;
        }

        std::cout << "Metal render pipeline state created successfully" << std::endl;

        // 主循环
        std::cout << "\nRendering with Metal..." << std::endl;
        std::cout << "Press ESC to exit" << std::endl;

        while (!glfwWindowShouldClose(window)) {
            // 处理输入
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, true);
            }

            // 开始帧
            context->BeginFrame();

            // 清除屏幕（深蓝色背景，展示Metal渲染）
            context->Clear(ClearFlag_Color | ClearFlag_Depth, 0.1f, 0.2f, 0.4f, 1.0f, 1.0f, 0);

            // 使用着色器程序
            shaderProgram->Use();

            // 设置管线状态
            context->SetPipelineState(pipelineState);

            // 绑定顶点缓冲区并绘制
            context->SetVertexBuffer(vertexBuffer, 0);
            context->Draw(0, 3);

            // 结束帧并呈现
            context->EndFrame();

            glfwPollEvents();
        }

        // 清理资源
        std::cout << "\nCleaning up..." << std::endl;

        delete pipelineState;
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
