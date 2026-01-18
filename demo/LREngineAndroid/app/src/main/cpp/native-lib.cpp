/**
 * @file native-lib.cpp
 * @brief LREngine Android JNI桥接层
 * 
 * 实现Android Java/Kotlin层与LREngine C++渲染引擎的交互
 */

#include <jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <GLES3/gl3.h>
#include <EGL/egl.h>

#include <cmath>
#include <cstring>

// LREngine头文件
#include <lrengine/core/LRRenderContext.h>
#include <lrengine/core/LRBuffer.h>
#include <lrengine/core/LRShader.h>
#include <lrengine/core/LRPipelineState.h>
#include <lrengine/core/LRTypes.h>
#include <lrengine/math/Vec3.hpp>
#include <lrengine/math/Mat4.hpp>
#include <lrengine/utils/LRLog.h>

#define LOG_TAG "LREngineJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

using namespace lrengine::render;
using namespace lrengine::utils;

// ===========================================================================
// LREngine日志回调 - 重定向到Android Logcat
// ===========================================================================

static void AndroidLogCallback(const LogEntry& entry) {
    int priority = ANDROID_LOG_INFO;
    switch (entry.level) {
        case LogLevel::Trace:   priority = ANDROID_LOG_VERBOSE; break;
        case LogLevel::Debug:   priority = ANDROID_LOG_DEBUG; break;
        case LogLevel::Info:    priority = ANDROID_LOG_INFO; break;
        case LogLevel::Warning: priority = ANDROID_LOG_WARN; break;
        case LogLevel::Error:   priority = ANDROID_LOG_ERROR; break;
        case LogLevel::Fatal:   priority = ANDROID_LOG_FATAL; break;
        default: break;
    }
    
    // 从文件路径提取文件名
    const char* fileName = entry.file.c_str();
    const char* lastSlash = fileName;
    for (const char* p = fileName; *p; ++p) {
        if (*p == '/' || *p == '\\') {
            lastSlash = p + 1;
        }
    }
    
    __android_log_print(priority, "LREngine", "[%s:%d] %s", 
                        lastSlash, entry.line, entry.message.c_str());
}

/**
 * @brief 初始化LREngine日志系统
 */
static void InitLREngineLog() {
    LRLog::Initialize();
    LRLog::SetMinLevel(LogLevel::Trace);  // 显示所有级别日志
    LRLog::EnableConsoleOutput(false);    // 禁用stderr输出（Android上无用）
    LRLog::SetLogCallback(AndroidLogCallback);  // 设置Android回调
    LOGI("LREngine log system initialized with Android callback");
}

using namespace lrengine::math;

// 类型别名
using Mat4 = Mat4T<float>;
using Vec3 = Vec3T<float>;

// ===========================================================================
// 全局变量
// ===========================================================================

// EGL相关
static EGLDisplay g_eglDisplay = EGL_NO_DISPLAY;
static EGLSurface g_eglSurface = EGL_NO_SURFACE;
static EGLContext g_eglContext = EGL_NO_CONTEXT;

// 渲染资源
static LRRenderContext* g_renderContext = nullptr;
static LRVertexBuffer* g_vertexBuffer = nullptr;
static LRIndexBuffer* g_indexBuffer = nullptr;
static LRShader* g_vertexShader = nullptr;
static LRShader* g_fragmentShader = nullptr;
static LRShaderProgram* g_shaderProgram = nullptr;
static LRPipelineState* g_pipelineState = nullptr;

// 渲染状态
static int g_width = 0;
static int g_height = 0;
static float g_rotationAngle = 0.0f;
static int g_frameCount = 0;

// ===========================================================================
// 着色器源码
// ===========================================================================

static const char* VERTEX_SHADER_SOURCE = R"(#version 300 es
precision highp float;

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;

out vec3 vColor;

uniform mat4 uMVP;

void main() {
    gl_Position = uMVP * vec4(aPosition, 1.0);
    vColor = aColor;
}
)";

static const char* FRAGMENT_SHADER_SOURCE = R"(#version 300 es
precision highp float;

in vec3 vColor;
out vec4 fragColor;

void main() {
    fragColor = vec4(vColor, 1.0);
}
)";

// ===========================================================================
// 立方体顶点数据
// ===========================================================================

// 立方体顶点数据：位置(xyz) + 颜色(rgb)
static const float CUBE_VERTICES[] = {
    // 前面 - 红色
    -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
    
    // 后面 - 绿色
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
    
    // 顶面 - 蓝色
    -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
    
    // 底面 - 黄色
    -0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
    
    // 右面 - 青色
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
    
    // 左面 - 品红色
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 1.0f
};

// 立方体索引数据
static const uint16_t CUBE_INDICES[] = {
    // 前面
    0, 1, 2, 0, 2, 3,
    // 后面
    4, 5, 6, 4, 6, 7,
    // 顶面
    8, 9, 10, 8, 10, 11,
    // 底面
    12, 13, 14, 12, 14, 15,
    // 右面
    16, 17, 18, 16, 18, 19,
    // 左面
    20, 21, 22, 20, 22, 23
};

// ===========================================================================
// 矩阵工具函数 - 使用LREngine数学库
// ===========================================================================

// PI常量
static constexpr float AAPI = 3.14159265358979323846f;

// ===========================================================================
// EGL初始化
// ===========================================================================

/**
 * @brief 初始化EGL上下文
 */
static bool InitEGL(ANativeWindow* window) {
    LOGI("Initializing EGL...");
    
    // 获取EGL显示
    g_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g_eglDisplay == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay failed");
        return false;
    }
    
    // 初始化EGL
    EGLint major, minor;
    if (!eglInitialize(g_eglDisplay, &major, &minor)) {
        LOGE("eglInitialize failed");
        return false;
    }
    LOGI("EGL initialized: version %d.%d", major, minor);
    
    // 配置属性
    const EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };
    
    // 选择配置
    EGLConfig config;
    EGLint numConfigs;
    if (!eglChooseConfig(g_eglDisplay, configAttribs, &config, 1, &numConfigs) || numConfigs == 0) {
        LOGE("eglChooseConfig failed");
        return false;
    }
    
    // 创建EGL Surface
    g_eglSurface = eglCreateWindowSurface(g_eglDisplay, config, window, nullptr);
    if (g_eglSurface == EGL_NO_SURFACE) {
        LOGE("eglCreateWindowSurface failed");
        return false;
    }
    
    // 上下文属性（OpenGL ES 3.0）
    const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    
    // 创建EGL上下文
    g_eglContext = eglCreateContext(g_eglDisplay, config, EGL_NO_CONTEXT, contextAttribs);
    if (g_eglContext == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext failed");
        return false;
    }
    
    // 绑定上下文
    if (!eglMakeCurrent(g_eglDisplay, g_eglSurface, g_eglSurface, g_eglContext)) {
        LOGE("eglMakeCurrent failed");
        return false;
    }
    
    LOGI("EGL initialized successfully");
    return true;
}

/**
 * @brief 销毁EGL上下文
 */
static void DestroyEGL() {
    if (g_eglDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(g_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        
        if (g_eglContext != EGL_NO_CONTEXT) {
            eglDestroyContext(g_eglDisplay, g_eglContext);
            g_eglContext = EGL_NO_CONTEXT;
        }
        
        if (g_eglSurface != EGL_NO_SURFACE) {
            eglDestroySurface(g_eglDisplay, g_eglSurface);
            g_eglSurface = EGL_NO_SURFACE;
        }
        
        eglTerminate(g_eglDisplay);
        g_eglDisplay = EGL_NO_DISPLAY;
    }
    
    LOGI("EGL destroyed");
}

// ===========================================================================
// 渲染资源初始化
// ===========================================================================

/**
 * @brief 初始化渲染资源
 */
static bool InitRenderResources() {
    LOGI("Initializing render resources...");
    
    // 创建渲染上下文
    RenderContextDescriptor contextDesc;
    contextDesc.backend = Backend::OpenGLES;
    contextDesc.width = g_width;
    contextDesc.height = g_height;
    contextDesc.vsync = true;
    contextDesc.debug = true;
    
    g_renderContext = LRRenderContext::Create(contextDesc);
    if (!g_renderContext) {
        LOGE("Failed to create render context");
        return false;
    }
    LOGI("Render context created");
    
    // 创建顶点缓冲区
    BufferDescriptor vbDesc;
    vbDesc.size = sizeof(CUBE_VERTICES);
    vbDesc.usage = BufferUsage::Static;
    vbDesc.type = BufferType::Vertex;
    vbDesc.data = CUBE_VERTICES;
    vbDesc.stride = 6 * sizeof(float); // 位置(3) + 颜色(3)
    vbDesc.debugName = "CubeVertexBuffer";
    
    g_vertexBuffer = g_renderContext->CreateVertexBuffer(vbDesc);
    if (!g_vertexBuffer) {
        LOGE("Failed to create vertex buffer");
        return false;
    }
    
    // 设置顶点布局 - 关键步骤！必须在创建后调用
    VertexLayoutDescriptor vertexLayout;
    vertexLayout.stride = 6 * sizeof(float);
    vertexLayout.attributes.push_back({
        0,                        // location - 位置
        VertexFormat::Float3,     // format
        0,                        // offset
        6 * sizeof(float),        // stride
        false                     // normalized
    });
    vertexLayout.attributes.push_back({
        1,                        // location - 颜色
        VertexFormat::Float3,     // format
        3 * sizeof(float),        // offset
        6 * sizeof(float),        // stride
        false                     // normalized
    });
    g_vertexBuffer->SetVertexLayout(vertexLayout);
    LOGI("Vertex buffer created and layout configured");
    
    // 创建索引缓冲区
    BufferDescriptor ibDesc;
    ibDesc.size = sizeof(CUBE_INDICES);
    ibDesc.usage = BufferUsage::Static;
    ibDesc.type = BufferType::Index;
    ibDesc.data = CUBE_INDICES;
    ibDesc.indexType = IndexType::UInt16;
    ibDesc.debugName = "CubeIndexBuffer";
    
    g_indexBuffer = g_renderContext->CreateIndexBuffer(ibDesc);
    if (!g_indexBuffer) {
        LOGE("Failed to create index buffer");
        return false;
    }
    LOGI("Index buffer created");
    
    // 创建顶点着色器
    ShaderDescriptor vsDesc;
    vsDesc.stage = ShaderStage::Vertex;
    vsDesc.language = ShaderLanguage::GLSL;
    vsDesc.source = VERTEX_SHADER_SOURCE;
    vsDesc.debugName = "CubeVertexShader";
    
    g_vertexShader = g_renderContext->CreateShader(vsDesc);
    if (!g_vertexShader) {
        LOGE("Failed to create vertex shader");
        return false;
    }
    LOGI("Vertex shader created");
    
    // 创建片段着色器
    ShaderDescriptor fsDesc;
    fsDesc.stage = ShaderStage::Fragment;
    fsDesc.language = ShaderLanguage::GLSL;
    fsDesc.source = FRAGMENT_SHADER_SOURCE;
    fsDesc.debugName = "CubeFragmentShader";
    
    g_fragmentShader = g_renderContext->CreateShader(fsDesc);
    if (!g_fragmentShader) {
        LOGE("Failed to create fragment shader");
        return false;
    }
    LOGI("Fragment shader created");
    
    // 创建着色器程序
    g_shaderProgram = g_renderContext->CreateShaderProgram(g_vertexShader, g_fragmentShader);
    if (!g_shaderProgram) {
        LOGE("Failed to create shader program");
        return false;
    }
    LOGI("Shader program created");
    
    // 创建管线状态
    PipelineStateDescriptor psoDesc;
    psoDesc.vertexShader = g_vertexShader;
    psoDesc.fragmentShader = g_fragmentShader;
    psoDesc.vertexLayout = vertexLayout;
    
    // 深度测试
    psoDesc.depthStencilState.depthTestEnabled = true;
    psoDesc.depthStencilState.depthWriteEnabled = true;
    psoDesc.depthStencilState.depthCompareFunc = CompareFunc::Less;
    
    // 背面剔除
    psoDesc.rasterizerState.cullMode = CullMode::Back;
    psoDesc.rasterizerState.frontFace = FrontFace::CCW;
    
    psoDesc.primitiveType = PrimitiveType::Triangles;
    psoDesc.debugName = "CubePipelineState";
    
    g_pipelineState = g_renderContext->CreatePipelineState(psoDesc);
    if (!g_pipelineState) {
        LOGE("Failed to create pipeline state");
        return false;
    }
    LOGI("Pipeline state created");
    
    LOGI("All render resources initialized successfully");
    return true;
}

/**
 * @brief 销毁渲染资源
 */
static void DestroyRenderResources() {
    // 注意：LREngine资源由上下文管理，销毁上下文会自动释放资源
    if (g_renderContext) {
        LRRenderContext::Destroy(g_renderContext);
        g_renderContext = nullptr;
    }
    
    g_vertexBuffer = nullptr;
    g_indexBuffer = nullptr;
    g_vertexShader = nullptr;
    g_fragmentShader = nullptr;
    g_shaderProgram = nullptr;
    g_pipelineState = nullptr;
    
    LOGI("Render resources destroyed");
}

// ===========================================================================
// JNI导出函数
// ===========================================================================

extern "C" {

/**
 * @brief 初始化渲染器
 * @param env JNI环境
 * @param thiz 调用对象
 * @param surface Android Surface对象
 * @param width 宽度
 * @param height 高度
 * @return 是否初始化成功
 */
JNIEXPORT jboolean JNICALL
Java_com_example_lrenginedemo_LREngineRenderer_nativeInit(
    JNIEnv* env,
    jobject /* thiz */,
    jobject surface,
    jint width,
    jint height) {
    
    LOGI("nativeInit called: %dx%d", width, height);
    
    // 初始化LREngine日志系统（重定向到Android Logcat）
    InitLREngineLog();
    
    g_width = width;
    g_height = height;
    
    // 获取ANativeWindow
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (!window) {
        LOGE("Failed to get ANativeWindow");
        return JNI_FALSE;
    }
    
    // 初始化EGL
    if (!InitEGL(window)) {
        ANativeWindow_release(window);
        return JNI_FALSE;
    }
    
    // 初始化渲染资源
    if (!InitRenderResources()) {
        DestroyEGL();
        ANativeWindow_release(window);
        return JNI_FALSE;
    }
    
    ANativeWindow_release(window);
    
    LOGI("nativeInit completed successfully");
    return JNI_TRUE;
}

/**
 * @brief 调整视口大小
 * @param env JNI环境
 * @param thiz 调用对象
 * @param width 新宽度
 * @param height 新高度
 */
JNIEXPORT void JNICALL
Java_com_example_lrenginedemo_LREngineRenderer_nativeResize(
    JNIEnv* /* env */,
    jobject /* thiz */,
    jint width,
    jint height) {
    
    LOGI("nativeResize: %dx%d", width, height);
    g_width = width;
    g_height = height;
    
    if (g_renderContext) {
        glViewport(0, 0, width, height);
    }
}

/**
 * @brief 渲染一帧
 * @param env JNI环境
 * @param thiz 调用对象
 */
JNIEXPORT void JNICALL
Java_com_example_lrenginedemo_LREngineRenderer_nativeRender(
    JNIEnv* /* env */,
    jobject /* thiz */) {
    
    if (!g_renderContext || !g_pipelineState || !g_vertexBuffer || !g_indexBuffer) {
        LOGW("nativeRender: resources not ready");
        return;
    }
    
    // 每100帧输出一次日志
    g_frameCount++;
    if (g_frameCount % 100 == 1) {
        LOGI("Rendering frame %d, size: %dx%d", g_frameCount, g_width, g_height);
    }
    
    // 更新旋转角度（每帧增加）
    g_rotationAngle += 0.02f;
    if (g_rotationAngle > 2.0f * 3.14159265f) {
        g_rotationAngle -= 2.0f * 3.14159265f;
    }
    
    // 开始帧
    g_renderContext->BeginFrame();
    
    // 开始渲染通道
    g_renderContext->BeginRenderPass(nullptr);
    
    // 清除缓冲区
    g_renderContext->Clear(ClearColor | ClearDepth, 0.15f, 0.15f, 0.9f, 1.0f, 1.0f, 0);
    
    // 设置视口
    g_renderContext->SetViewport(0, 0, g_width, g_height);
    
    // 设置管线状态
    g_renderContext->SetPipelineState(g_pipelineState);
    
    // 设置顶点和索引缓冲区
    g_renderContext->SetVertexBuffer(g_vertexBuffer, 0);
    g_renderContext->SetIndexBuffer(g_indexBuffer);
    
    // 计算MVP矩阵 - 使用LREngine的Mat4T类
    float aspect = static_cast<float>(g_width) / static_cast<float>(g_height);
    float fovY = 45.0f * AAPI / 180.0f;
    
    // 使用LREngine的矩阵工厂方法
    Mat4 projection = Mat4::perspective(fovY, aspect, 0.1f, 100.0f);
    Mat4 view = Mat4::lookAt(Vec3(0.0f, 0.0f, 6.0f), Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));
    Mat4 modelY = Mat4::rotateY(g_rotationAngle);
    Mat4 modelX = Mat4::rotateX(g_rotationAngle * 0.5f);
    Mat4 model = modelY * modelX;
    Mat4 mvp = projection * view * model;
    
    // 设置MVP矩阵Uniform
    g_pipelineState->GetShaderProgram()->SetUniformMatrix4("uMVP", mvp, false);
    

    // 检查深度测试状态
    GLboolean depthTestEnabled = GL_FALSE;
    GLboolean depthWriteEnabled = GL_FALSE;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWriteEnabled);
    LOGW("Depth: test=%d, write=%d", depthTestEnabled, depthWriteEnabled);


    // 绘制立方体
    g_renderContext->DrawIndexed(0, 36);
    
    // 结束渲染通道
    g_renderContext->EndRenderPass();
    
    // 结束帧
    g_renderContext->EndFrame();
    
    // 交换缓冲区
    if (!eglSwapBuffers(g_eglDisplay, g_eglSurface)) {
        LOGE("eglSwapBuffers failed: 0x%04x", eglGetError());
    }
}

/**
 * @brief 销毁渲染器
 * @param env JNI环境
 * @param thiz 调用对象
 */
JNIEXPORT void JNICALL
Java_com_example_lrenginedemo_LREngineRenderer_nativeDestroy(
    JNIEnv* /* env */,
    jobject /* thiz */) {
    
    LOGI("nativeDestroy called");
    
    DestroyRenderResources();
    DestroyEGL();
    
    g_width = 0;
    g_height = 0;
    g_rotationAngle = 0.0f;
    
    LOGI("nativeDestroy completed");
}

} // extern "C"
