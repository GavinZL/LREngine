/**
 * @file main.cpp
 * @brief HelloTriangle示例 - LREngine基本渲染演示
 * 
 * 演示如何使用LREngine渲染一个简单的彩色三角形
 */

#define GL_SILENCE_DEPRECATION

#include <iostream>
#include <GLFW/glfw3.h>

#include "lrengine/core/LRDefines.h"
#include "lrengine/core/LRTypes.h"
#include "lrengine/core/LRError.h"
#include "lrengine/core/LRBuffer.h"
#include "lrengine/core/LRShader.h"
#include "lrengine/core/LRRenderContext.h"
#include "lrengine/factory/LRDeviceFactory.h"

using namespace lrengine::render;

// 窗口尺寸
constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;

// 顶点着色器源码
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 vertexColor;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    vertexColor = aColor;
}
)";

// 片段着色器源码
const char* fragmentShaderSource = R"(
#version 330 core
in vec3 vertexColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(vertexColor, 1.0);
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
    glViewport(0, 0, width, height);
}

int main()
{
    std::cout << "LREngine HelloTriangle Example" << std::endl;
    std::cout << "===============================" << std::endl;

    // 设置错误回调
    LRError::SetErrorCallback(errorCallback);
    glfwSetErrorCallback(glfwErrorCallback);

    // 初始化GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // 设置OpenGL版本
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef LR_PLATFORM_APPLE
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // 创建窗口
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, 
                                          "LREngine - HelloTriangle", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // 启用垂直同步
    glfwSwapInterval(1);

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    // 创建渲染上下文
    RenderContextDescriptor contextDesc;
    contextDesc.backend = Backend::OpenGL;
    contextDesc.width = WINDOW_WIDTH;
    contextDesc.height = WINDOW_HEIGHT;
    contextDesc.windowHandle = window;

    auto context = LRRenderContext::Create(contextDesc);
    if (!context) {
        std::cerr << "Failed to create render context" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    std::cout << "Render context created successfully" << std::endl;

    // 创建顶点着色器
    ShaderDescriptor vsDesc;
    vsDesc.stage = ShaderStage::Vertex;
    vsDesc.source = vertexShaderSource;
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

    std::cout << "Vertex shader compiled successfully" << std::endl;

    // 创建片段着色器
    ShaderDescriptor fsDesc;
    fsDesc.stage = ShaderStage::Fragment;
    fsDesc.source = fragmentShaderSource;
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

    std::cout << "Fragment shader compiled successfully" << std::endl;

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

    std::cout << "Shader program linked successfully" << std::endl;

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

    std::cout << "Vertex buffer created successfully" << std::endl;

    // 主循环
    std::cout << "\nRendering..." << std::endl;
    std::cout << "Press ESC to exit" << std::endl;

    while (!glfwWindowShouldClose(window)) {
        // 处理输入
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        // 清除屏幕
        context->Clear(ClearFlag_Color | ClearFlag_Depth, 0.2f, 0.3f, 0.3f, 1.0f, 1.0f, 0);

        // 使用着色器程序
        shaderProgram->Use();

        // 绑定顶点缓冲区并绘制
        context->SetVertexBuffer(vertexBuffer, 0);
        context->Draw(0, 3);

        // 交换缓冲区
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 清理资源
    std::cout << "\nCleaning up..." << std::endl;

    // 手动删除资源（或使用LRRenderContext::Destroy方法）
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
