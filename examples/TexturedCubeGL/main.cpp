/**
 * @file main.cpp
 * @brief TexturedCubeGL示例 - OpenGL渲染带纹理的3D立方体
 * 
 * 演示如何使用LREngine的OpenGL后端实现：
 * - 3D立方体几何体的创建
 * - 纹理加载和采样
 * - 3D变换矩阵（MVP）
 * - 旋转动画
 */

#include <iostream>
#include <cmath>
#include <vector>
#include <cstddef>
#include <GLFW/glfw3.h>

#include "lrengine/core/LRDefines.h"
#include "lrengine/core/LRTypes.h"
#include "lrengine/core/LRError.h"
#include "lrengine/core/LRBuffer.h"
#include "lrengine/core/LRShader.h"
#include "lrengine/core/LRTexture.h"
#include "lrengine/core/LRRenderContext.h"
#include "lrengine/core/LRPipelineState.h"
#include "lrengine/factory/LRDeviceFactory.h"
#include "lrengine/utils/LRLog.h"
#include "lrengine/math/Vec2.hpp"
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

// OpenGL着色器源码 (GLSL)
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec2 TexCoord;
out vec3 Normal;
out vec3 WorldPos;

void main()
{
    mat4 mvp = projectionMatrix * viewMatrix * modelMatrix;
    gl_Position = mvp * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
    Normal = mat3(modelMatrix) * aNormal;
    WorldPos = vec3(modelMatrix * vec4(aPos, 1.0));
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 WorldPos;

uniform sampler2D colorTexture;

void main()
{
    // 纹理采样
    vec4 texColor = texture(colorTexture, TexCoord);
    
    // 简单的方向光照
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 normal = normalize(Normal);
    float diffuse = max(dot(normal, lightDir), 0.0);
    
    // 环境光 + 漫反射
    vec3 ambient = vec3(0.3, 0.3, 0.3);
    vec3 lighting = ambient + diffuse * 0.7;
    
    FragColor = vec4(texColor.rgb * lighting, texColor.a);
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
    // 初始化日志系统
    lrengine::utils::LRLog::Initialize();
    lrengine::utils::LRLog::SetMinLevel(lrengine::utils::LogLevel::Trace);
    lrengine::utils::LRLog::EnableConsoleOutput(true);

    LR_LOG_INFO("LREngine TexturedCubeGL Example Starting...");
    std::cout << "LREngine TexturedCubeGL Example" << std::endl;
    std::cout << "================================" << std::endl;
    std::cout << "Demonstrating:" << std::endl;
    std::cout << "  - 3D textured cube rendering with OpenGL" << std::endl;
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

    // 设置OpenGL版本
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // 创建窗口
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, 
                                          "LREngine - TexturedCubeGL", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // 获取实际的帧缓冲尺寸（解决 Retina 屏幕下视口只有四分之一的问题）
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

    // 创建渲染上下文
    RenderContextDescriptor contextDesc;
    contextDesc.backend = Backend::OpenGL;
    contextDesc.width = fbWidth;
    contextDesc.height = fbHeight;
    contextDesc.windowHandle = window;
    contextDesc.vsync = true;

    auto context = LRRenderContext::Create(contextDesc);
    if (!context) {
        std::cerr << "Failed to create OpenGL render context" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    std::cout << "OpenGL render context created successfully" << std::endl;

    // 创建顶点着色器
    ShaderDescriptor vsDesc;
    vsDesc.stage = ShaderStage::Vertex;
    vsDesc.language = ShaderLanguage::GLSL;
    vsDesc.source = vertexShaderSource;
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
    fsDesc.language = ShaderLanguage::GLSL;
    fsDesc.source = fragmentShaderSource;
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
    const char* texturePath = "resources/MG_1822.png";
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
    }

    TextureDescriptor texDesc;
    texDesc.type = TextureType::Texture2D;
    texDesc.width = texWidth;
    texDesc.height = texHeight;
    texDesc.format = PixelFormat::RGBA8;
    texDesc.data = texData;
    texDesc.debugName = "CubeTexture";
    
    // 配置采样器
    texDesc.sampler.minFilter = FilterMode::Linear;
    texDesc.sampler.magFilter = FilterMode::Linear;
    texDesc.sampler.wrapU = WrapMode::Repeat;
    texDesc.sampler.wrapV = WrapMode::Repeat;

    LRTexture* texture = context->CreateTexture(texDesc);
    
    if (texData) {
        stbi_image_free(texData);
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
    
    // 暂时关闭剔除以进行调试，或者使用与Metal一致的设置
    pipelineDesc.rasterizerState.cullMode = CullMode::Back; 
    pipelineDesc.rasterizerState.frontFace = FrontFace::CCW;
    
    pipelineDesc.debugName = "CubePipeline";

    LRPipelineState* pipelineState = context->CreatePipelineState(pipelineDesc);
    if (!pipelineState) {
        std::cerr << "Failed to create pipeline state" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    std::cout << "Pipeline state created successfully" << std::endl;

    // 主循环
    std::cout << "\nRendering textured cube with OpenGL..." << std::endl;
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
        Mat4 model = Mat4::rotateY(rotationAngle) * Mat4::rotateX(rotationAngle * 0.5f);
        
        Vec3 eye(0.0f, 0.0f, 10.0f);
        Vec3 center(0.0f, 0.0f, 0.0f);
        Vec3 up(0.0f, 1.0f, 0.0f);
        Mat4 view = Mat4::lookAt(eye, center, up);
        
        float aspect = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);
        Mat4 projection = Mat4::perspective(45.0f * 3.14159f / 180.0f, aspect, 0.1f, 100.0f);

        // 开始帧
        context->BeginFrame();
        LR_LOG_TRACE("Frame Begin");

        // 清除屏幕
        context->Clear(ClearFlag_Color | ClearFlag_Depth, 0.2f, 0.3f, 0.4f, 1.0f, 1.0f, 0);

        // 设置管线状态 (这会绑定 pipelineState 内部的着色器程序)
        context->SetPipelineState(pipelineState);

        // 设置Uniform (必须在程序绑定后设置)
        shaderProgram->SetUniformMatrix4("modelMatrix", model, true);
        shaderProgram->SetUniformMatrix4("viewMatrix", view, true);
        shaderProgram->SetUniformMatrix4("projectionMatrix", projection, false);
        shaderProgram->SetUniform("colorTexture", 0); // 纹理槽位0

        // 绑定资源
        context->SetVertexBuffer(vertexBuffer, 0);
        context->SetTexture(texture, 0);

        // 绘制立方体
        context->Draw(0, 36);
        
        static bool firstFrame = true;
        if (firstFrame) {
            LR_LOG_INFO("First frame draw call submitted");
            firstFrame = false;
        }

        // 呈现帧
        glfwSwapBuffers(window);
        glfwPollEvents();
        
        LR_LOG_TRACE("Frame End");
    }

    // 清理资源
    std::cout << "\nCleaning up..." << std::endl;

    delete pipelineState;
    delete texture;
    delete vertexBuffer;
    delete fragmentShader;
    delete vertexShader;
    LRRenderContext::Destroy(context);

    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "Done!" << std::endl;
    return 0;
}
