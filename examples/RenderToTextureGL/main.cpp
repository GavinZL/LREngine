/**
 * @file main.cpp
 * @brief RenderToTextureGL示例 - OpenGL实现离屏渲染和全屏显示
 * 
 * 演示如何使用LREngine的OpenGL后端实现：
 * - 将三个立方体渲染到离屏纹理（render-to-texture）
 * - 将离屏纹理渲染到全屏显示
 * - FrameBuffer的使用
 * - 多pass渲染流程
 */

#include <iostream>
#include <cmath>
#include <GLFW/glfw3.h>

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
#include "lrengine/math/MathFwd.hpp"
#include "lrengine/utils/LRLog.h"
#include "lrengine/math/Vec2.hpp"
#include "lrengine/math/Vec3.hpp"
#include "lrengine/math/Vec4.hpp"
#include "lrengine/math/Mat4.hpp"

// stb_image for texture loading
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

using namespace lrengine::render;
using namespace lrengine::math;

// OpenGL错误检查辅助函数
void CheckGLError(const char* location) {
    GLenum err;
    bool hasError = false;
    while ((err = glGetError()) != GL_NO_ERROR) {
        hasError = true;
        const char* errorStr = "Unknown error";
        switch(err) {
            case GL_INVALID_ENUM:      errorStr = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE:     errorStr = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION: errorStr = "GL_INVALID_OPERATION"; break;
            case GL_OUT_OF_MEMORY:     errorStr = "GL_OUT_OF_MEMORY"; break;
            #ifdef GL_INVALID_FRAMEBUFFER_OPERATION
            case GL_INVALID_FRAMEBUFFER_OPERATION: errorStr = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            #endif
        }
        LR_LOG_ERROR_F("[GL Error] %s - Error: 0x%04X (%s)", location, err, errorStr);
        std::cerr << "[GL Error] " << location << " - Error: 0x" << std::hex << err << " (" << errorStr << ")" << std::dec << std::endl;
    }
    if (!hasError) {
        LR_LOG_TRACE_F("[GL Check] %s - OK", location);
    }
}

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

// Quad渲染的顶点结构
struct QuadVertex {
    Vec3f position;
    Vec2f texCoord;
};

// OpenGL着色器源码 - Cube渲染
const char* cubeVertexShaderSource = R"(
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

const char* cubeFragmentShaderSource = R"(
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

// OpenGL着色器源码 - Quad全屏显示
const char* quadVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

const char* quadFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D colorTexture;

void main()
{
    FragColor = texture(colorTexture, TexCoord);
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
QuadVertex quadVertices[] = {
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},  // 左下
    {{ 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},  // 右下
    {{ 1.0f,  1.0f, 0.0f}, {1.0f, 1.0f}},  // 右上
    {{ 1.0f,  1.0f, 0.0f}, {1.0f, 1.0f}},  // 右上
    {{-1.0f,  1.0f, 0.0f}, {0.0f, 1.0f}},  // 左上
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},  // 左下
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

    LR_LOG_INFO("========================================");
    LR_LOG_INFO("LREngine RenderToTextureGL Example Starting...");
    LR_LOG_INFO("========================================");
    std::cout << "LREngine RenderToTextureGL Example" << std::endl;
    std::cout << "===================================" << std::endl;
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

    // 设置OpenGL版本
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // 创建窗口
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, 
                                          "LREngine - RenderToTextureGL", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // 获取实际的帧缓冲尺寸（Retina屏幕可能与窗口尺寸不同）
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    
    std::cout << "Window Size: " << WINDOW_WIDTH << "x" << WINDOW_HEIGHT << std::endl;
    std::cout << "Framebuffer Size: " << fbWidth << "x" << fbHeight << std::endl;
    LR_LOG_INFO_F("Window: %dx%d, Framebuffer: %dx%d", WINDOW_WIDTH, WINDOW_HEIGHT, fbWidth, fbHeight);

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
    LR_LOG_INFO_F("OpenGL Context Created: %dx%d", fbWidth, fbHeight);
    CheckGLError("After Context Creation");

    // ========== 创建Cube渲染的着色器 ==========
    ShaderDescriptor cubeVsDesc;
    cubeVsDesc.stage = ShaderStage::Vertex;
    cubeVsDesc.language = ShaderLanguage::GLSL;
    cubeVsDesc.source = cubeVertexShaderSource;
    cubeVsDesc.debugName = "CubeVS";

    LRShader* cubeVertexShader = context->CreateShader(cubeVsDesc);
    if (!cubeVertexShader || !cubeVertexShader->IsCompiled()) {
        std::cerr << "Failed to compile cube vertex shader" << std::endl;
        return -1;
    }

    ShaderDescriptor cubeFsDesc;
    cubeFsDesc.stage = ShaderStage::Fragment;
    cubeFsDesc.language = ShaderLanguage::GLSL;
    cubeFsDesc.source = cubeFragmentShaderSource;
    cubeFsDesc.debugName = "CubeFS";

    LRShader* cubeFragmentShader = context->CreateShader(cubeFsDesc);
    if (!cubeFragmentShader || !cubeFragmentShader->IsCompiled()) {
        std::cerr << "Failed to compile cube fragment shader" << std::endl;
        return -1;
    }

    std::cout << "Cube shaders compiled successfully" << std::endl;
    LR_LOG_INFO_F("Cube Shaders Compiled: VS=%p, FS=%p", (void*)cubeVertexShader, (void*)cubeFragmentShader);
    CheckGLError("After Cube Shader Compilation");

    // ========== 创建Quad渲染的着色器 ==========
    ShaderDescriptor quadVsDesc;
    quadVsDesc.stage = ShaderStage::Vertex;
    quadVsDesc.language = ShaderLanguage::GLSL;
    quadVsDesc.source = quadVertexShaderSource;
    quadVsDesc.debugName = "QuadVS";

    LRShader* quadVertexShader = context->CreateShader(quadVsDesc);
    if (!quadVertexShader || !quadVertexShader->IsCompiled()) {
        std::cerr << "Failed to compile quad vertex shader" << std::endl;
        return -1;
    }

    ShaderDescriptor quadFsDesc;
    quadFsDesc.stage = ShaderStage::Fragment;
    quadFsDesc.language = ShaderLanguage::GLSL;
    quadFsDesc.source = quadFragmentShaderSource;
    quadFsDesc.debugName = "QuadFS";

    LRShader* quadFragmentShader = context->CreateShader(quadFsDesc);
    if (!quadFragmentShader || !quadFragmentShader->IsCompiled()) {
        std::cerr << "Failed to compile quad fragment shader" << std::endl;
        return -1;
    }

    std::cout << "Quad shaders compiled successfully" << std::endl;
    LR_LOG_INFO_F("Quad Shaders Compiled: VS=%p, FS=%p", (void*)quadVertexShader, (void*)quadFragmentShader);
    CheckGLError("After Quad Shader Compilation");

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
    LR_LOG_INFO_F("CubeVB created: %zu vertices", sizeof(cubeVertices) / sizeof(Vertex));
    LR_LOG_INFO_F("QuadVB created: %zu vertices", sizeof(quadVertices) / sizeof(QuadVertex));
    CheckGLError("After Vertex Buffer Creation");

    // ========== 加载立方体纹理 ==========
    const char* texturePath = "resources/MG_1822.png";
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
    LR_LOG_INFO_F("Cube Texture Created: %dx%d, ID=%p", texWidth, texHeight, (void*)cubeTexture);
    CheckGLError("After Cube Texture Creation");

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
    
    if (!offscreenFrameBuffer->IsComplete()) {
        std::cerr << "Offscreen framebuffer is not complete" << std::endl;
        return -1;
    }

    std::cout << "Offscreen framebuffer created successfully" << std::endl;
    LR_LOG_INFO_F("Offscreen FBO Created: %dx%d", OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
    LR_LOG_INFO_F("  - FrameBuffer Object: %p", (void*)offscreenFrameBuffer);
    LR_LOG_INFO_F("  - Color Texture (LRTexture*): %p", (void*)offscreenColorTexture);
    LR_LOG_INFO_F("  - Color Texture NativeHandle: %p", offscreenColorTexture->GetNativeHandle().ptr);
    LR_LOG_INFO_F("  - Depth Texture (LRTexture*): %p", (void*)offscreenDepthTexture);
    LR_LOG_INFO_F("  - Depth Texture NativeHandle: %p", offscreenDepthTexture->GetNativeHandle().ptr);
    LR_LOG_INFO_F("  - FBO Complete: %d", offscreenFrameBuffer->IsComplete());
    CheckGLError("After Offscreen FBO Creation");

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
    LR_LOG_INFO_F("Cube Pipeline State Created: ID=%p, ShaderProgram=%p", 
                  (void*)cubePipelineState, (void*)cubePipelineState->GetShaderProgram());
    LR_LOG_INFO_F("Quad Pipeline State Created: ID=%p, ShaderProgram=%p", 
                  (void*)quadPipelineState, (void*)quadPipelineState->GetShaderProgram());
    CheckGLError("After Pipeline State Creation");

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
        LR_LOG_TRACE_F("========== Frame %.3f Begin ==========", rotationAngle);

        // ========== Pass 1: 将三个立方体渲染到离屏纹理 ==========
        LR_LOG_INFO("[Pass 1] === Rendering cubes to offscreen texture ===");
        LR_LOG_INFO_F("[Pass 1] Target FrameBuffer: %p", (void*)offscreenFrameBuffer);
        LR_LOG_INFO_F("[Pass 1] Target Color Texture (LRTexture*): %p", (void*)offscreenColorTexture);
        LR_LOG_INFO_F("[Pass 1] Target Color Texture NativeHandle: %p", offscreenColorTexture->GetNativeHandle().ptr);
        
        // 先绑定FBO，再清除
        context->BeginRenderPass(offscreenFrameBuffer);
        CheckGLError("[Pass 1] After BeginRenderPass");
        
        context->Clear(ClearFlag_Color | ClearFlag_Depth, 0.1f, 0.1f, 0.15f, 1.0f, 1.0f, 0);
        LR_LOG_INFO("[Pass 1] Cleared to color (0.1, 0.1, 0.15)");
        CheckGLError("[Pass 1] After Clear");
        
        // 注意：SetPipelineState 会自动调用 ShaderProgram->Use()
        context->SetPipelineState(cubePipelineState);
        CheckGLError("[Pass 1] After SetPipelineState");
        
        context->SetVertexBuffer(cubeVertexBuffer, 0);
        CheckGLError("[Pass 1] After SetVertexBuffer");
        
        context->SetTexture(cubeTexture, 0);
        CheckGLError("[Pass 1] After SetTexture");

        // 共享的视图和投影矩阵
        // 调整摄像机距离，让立方体更清晰可见
        Vec3f eye(0.0f, 0.0f, 8.0f);  // 从 20.0f 调整为 8.0f，更靠近立方体
        Vec3f center(0.0f, 0.0f, 0.0f);
        Vec3f up(0.0f, 1.0f, 0.0f);
        Mat4f viewMatrix = Mat4f::lookAt(eye, center, up);
        
        float aspect = static_cast<float>(OFFSCREEN_WIDTH) / static_cast<float>(OFFSCREEN_HEIGHT);
        Mat4f projectionMatrix = Mat4f::perspective(45.0f * 3.14159f / 180.0f, aspect, 0.1f, 100.0f);
        LR_LOG_TRACE("[Pass 1] View and Projection matrices computed (camera at Z=8.0)");

        // 绘制第一个立方体（左侧，绕Y轴旋转）
        {
            Mat4f translation = Mat4f::translate(Vec3f(-1.5f, 0.0f, 0.0f));  // 从 -2.5 调整为 -1.5
            Mat4f rotation = Mat4f::rotateY(rotationAngle);
            Mat4f modelMatrix = translation * rotation;
            
            cubePipelineState->GetShaderProgram()->SetUniformMatrix4("modelMatrix", modelMatrix, true);
            cubePipelineState->GetShaderProgram()->SetUniformMatrix4("viewMatrix", viewMatrix, true);
            cubePipelineState->GetShaderProgram()->SetUniformMatrix4("projectionMatrix", projectionMatrix, false);
            cubePipelineState->GetShaderProgram()->SetUniform("colorTexture", 0);
            
            context->Draw(0, 36);
            LR_LOG_TRACE("[Pass 1] Cube 1 drawn (left, Y-rotation)");
            CheckGLError("[Pass 1] After Draw Cube 1");
        }

        // 绘制第二个立方体（中间，绕X轴旋转，稍微放大）
        {
            Mat4f scale = Mat4f::scale(Vec3f(1.3f, 1.3f, 1.3f));
            Mat4f rotation = Mat4f::rotateX(rotationAngle * 1.5f);
            Mat4f modelMatrix = scale * rotation;
            
            cubePipelineState->GetShaderProgram()->SetUniformMatrix4("modelMatrix", modelMatrix, true);
            cubePipelineState->GetShaderProgram()->SetUniformMatrix4("viewMatrix", viewMatrix, true);
            cubePipelineState->GetShaderProgram()->SetUniformMatrix4("projectionMatrix", projectionMatrix, false);
            cubePipelineState->GetShaderProgram()->SetUniform("colorTexture", 0);
            
            context->Draw(0, 36);
            LR_LOG_TRACE("[Pass 1] Cube 2 drawn (center, X-rotation, scaled 1.3x)");
            CheckGLError("[Pass 1] After Draw Cube 2");
        }

        // 绘制第三个立方体（右侧，绕Z轴旋转，缩小）
        {
            Mat4f translation = Mat4f::translate(Vec3f(1.5f, 0.0f, 0.0f));  // 从 2.5 调整为 1.5
            Mat4f scale = Mat4f::scale(Vec3f(0.7f, 0.7f, 0.7f));
            Mat4f rotation = Mat4f::rotateZ(rotationAngle * 2.0f);
            Mat4f modelMatrix = translation * scale * rotation;
            
            cubePipelineState->GetShaderProgram()->SetUniformMatrix4("modelMatrix", modelMatrix, true);
            cubePipelineState->GetShaderProgram()->SetUniformMatrix4("viewMatrix", viewMatrix, true);
            cubePipelineState->GetShaderProgram()->SetUniformMatrix4("projectionMatrix", projectionMatrix, false);
            cubePipelineState->GetShaderProgram()->SetUniform("colorTexture", 0);
            
            context->Draw(0, 36);
            LR_LOG_TRACE("[Pass 1] Cube 3 drawn (right, Z-rotation, scaled 0.7x)");
            CheckGLError("[Pass 1] After Draw Cube 3");
        }
        
        context->EndRenderPass();
        LR_LOG_TRACE("[Pass 1] EndRenderPass - Offscreen rendering complete");
        CheckGLError("[Pass 1] After EndRenderPass");
        
        // ========== Pass 2: 将离屏纹理渲染到屏幕 ==========
        LR_LOG_INFO("[Pass 2] === Rendering offscreen texture to screen ===");
        LR_LOG_INFO_F("[Pass 2] Source Color Texture (LRTexture*): %p", (void*)offscreenColorTexture);
        LR_LOG_INFO_F("[Pass 2] Source Color Texture NativeHandle: %p", offscreenColorTexture->GetNativeHandle().ptr);
        
        context->BeginRenderPass(nullptr);  // 绑定到默认帧缓冲（屏幕）
        CheckGLError("[Pass 2] After BeginRenderPass");
        
        // 不清除颜色，直接用Quad覆盖整个屏幕
        // 注意：也不需要清除深度，因为Quad的深度测试已禁用
        // context->Clear(ClearFlag_Depth, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0);
        
        // 注意：SetPipelineState 会自动调用 ShaderProgram->Use()
        context->SetPipelineState(quadPipelineState);
        CheckGLError("[Pass 2] After SetPipelineState");
        
        context->SetVertexBuffer(quadVertexBuffer, 0);
        CheckGLError("[Pass 2] After SetVertexBuffer");
        
        context->SetTexture(offscreenColorTexture, 0);  // 使用离屏渲染的纹理
        LR_LOG_INFO_F("[Pass 2] Texture Bound - LRTexture*: %p, NativeHandle: %p", 
                      (void*)offscreenColorTexture, offscreenColorTexture->GetNativeHandle().ptr);
        CheckGLError("[Pass 2] After SetTexture");
        
        quadPipelineState->GetShaderProgram()->SetUniform("colorTexture", 0);
        LR_LOG_TRACE("[Pass 2] Uniform 'colorTexture' Set to 0");
        
        context->Draw(0, 6);
        LR_LOG_TRACE("[Pass 2] Draw Quad (6 vertices)");
        CheckGLError("[Pass 2] After Draw Quad");
        
        context->EndRenderPass();
        LR_LOG_TRACE("[Pass 2] EndRenderPass - Screen rendering complete");
        CheckGLError("[Pass 2] After EndRenderPass");

        // 结束帧并呈现
        glfwSwapBuffers(window);
        CheckGLError("After SwapBuffers");
        glfwPollEvents();
        
        static int frameCount = 0;
        if (frameCount < 5 || frameCount % 60 == 0) {
            LR_LOG_INFO_F("Frame %d completed (rotation=%.3f)", frameCount, rotationAngle);
        }
        frameCount++;
    }

    // 清理资源
    std::cout << "\nCleaning up..." << std::endl;

    delete offscreenFrameBuffer;
    delete offscreenDepthTexture;
    delete offscreenColorTexture;
    delete cubeTexture;
    delete quadPipelineState;
    delete cubePipelineState;
    delete quadVertexBuffer;
    delete cubeVertexBuffer;
    delete quadFragmentShader;
    delete quadVertexShader;
    delete cubeFragmentShader;
    delete cubeVertexShader;
    LRRenderContext::Destroy(context);

    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "Done!" << std::endl;
    return 0;
}
