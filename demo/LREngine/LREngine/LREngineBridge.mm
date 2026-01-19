//
//  LREngineBridge.mm
//  LREngine
//
//  Created on 2026/1/17.
//  Objective-C++ 桥接层实现 - 封装LREngine C++接口供Swift调用
//  支持 Metal 和 OpenGL ES 双后端
//

#import "LREngineBridge.h"
#import <QuartzCore/QuartzCore.h>
#import <CoreGraphics/CoreGraphics.h>
#import <ImageIO/ImageIO.h>

// 引入LREngine C++头文件
#include <functional>
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
#include "lrengine/math/Vec2.hpp"
#include "lrengine/math/Mat4.hpp"

#include <iostream>
#include <cmath>

using namespace lrengine::render;
using namespace lrengine::math;

// 顶点结构
struct Vertex {
    Vec3f position;
    Vec2f texCoord;
    Vec3f normal;
};

// Uniform数据结构
struct Uniforms {
    Mat4f modelMatrix;
    Mat4f viewMatrix;
    Mat4f projectionMatrix;
};

// Metal着色器源码
static const char* metalShaderSource = R"(
#include <metal_stdlib>
using namespace metal;

struct Uniforms {
    float4x4 modelMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
};

struct VertexIn {
    float3 position [[attribute(0)]];
    float2 texCoord [[attribute(1)]];
    float3 normal [[attribute(2)]];
};

struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
    float3 normal;
    float3 worldPos;
};

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

// OpenGL ES 着色器源码 - 顶点着色器
static const char* glesVertexShaderSource = R"(
#version 300 es
precision highp float;

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;

out vec2 vTexCoord;
out vec3 vNormal;
out vec3 vWorldPos;

uniform mat4 uModelMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;

void main() {
    mat4 mvp = uProjectionMatrix * uViewMatrix * uModelMatrix;
    gl_Position = mvp * vec4(aPosition, 1.0);
    vTexCoord = aTexCoord;
    vNormal = mat3(uModelMatrix) * aNormal;
    vWorldPos = (uModelMatrix * vec4(aPosition, 1.0)).xyz;
}
)";

// OpenGL ES 着色器源码 - 片段着色器
static const char* glesFragmentShaderSource = R"(
#version 300 es
precision highp float;

in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vWorldPos;

out vec4 fragColor;

uniform sampler2D uTexture;

void main() {
    // 纹理采样
    vec4 texColor = texture(uTexture, vTexCoord);
    
    // 简单的方向光照
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 normal = normalize(vNormal);
    float diffuse = max(dot(normal, lightDir), 0.0);
    
    // 环境光 + 漫反射
    vec3 ambient = vec3(0.3, 0.3, 0.3);
    vec3 lighting = ambient + diffuse * 0.7;
    
    fragColor = vec4(texColor.rgb * lighting, texColor.a);
}
)";

// 立方体顶点数据（36个顶点，6个面）
static Vertex cubeVertices[] = {
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

@implementation LREngineRenderer {
    @private
    LRRenderBackend _backend;
    LRRenderContext* _context;
    LRShader* _vertexShader;
    LRShader* _fragmentShader;
    LRShaderProgram* _shaderProgram;
    LRVertexBuffer* _vertexBuffer;
    LRUniformBuffer* _uniformBuffer;
    LRPipelineState* _pipelineState;
    LRTexture* _texture;
    
    float _rotationAngle;
    NSInteger _width;
    NSInteger _height;
    
#if TARGET_OS_IPHONE
    // GLES 特有资源
    EAGLContext* _eaglContext;
    CAEAGLLayer* _eaglLayer;
    GLuint _colorRenderbuffer;
    GLuint _depthRenderbuffer;
    GLuint _framebuffer;
#endif
}

@synthesize backend = _backend;

- (instancetype)initWithMetalLayer:(CAMetalLayer *)metalLayer
                             width:(NSInteger)width
                            height:(NSInteger)height {
    self = [super init];
    if (self) {
        _backend = LRRenderBackendMetal;
        _width = width;
        _height = height;
        _rotationAngle = 0.0f;
        
        // 初始化LREngine
        if (![self setupLREngineWithMetalLayer:metalLayer]) {
            NSLog(@"Failed to setup LREngine with Metal");
            return nil;
        }
    }
    return self;
}

#if TARGET_OS_IPHONE
- (instancetype)initWithEAGLLayer:(CAEAGLLayer *)eaglLayer
                            width:(NSInteger)width
                           height:(NSInteger)height {
    self = [super init];
    if (self) {
        _backend = LRRenderBackendOpenGLES;
        _width = width;
        _height = height;
        _rotationAngle = 0.0f;
        _eaglLayer = eaglLayer;
        
        // 初始化LREngine
        if (![self setupLREngineWithEAGLLayer:eaglLayer]) {
            NSLog(@"Failed to setup LREngine with OpenGL ES");
            return nil;
        }
    }
    return self;
}
#endif

- (BOOL)setupLREngineWithMetalLayer:(CAMetalLayer *)metalLayer {
    @autoreleasepool {
        // 设置错误回调
        LRError::SetErrorCallback([](const ErrorInfo& error) {
            NSLog(@"[LREngine Error] %s (Code: %d) at %s:%d",
                  error.message.c_str(), static_cast<int>(error.code), 
                  error.file.c_str(), error.line);
        });
        
        // 创建渲染上下文
        RenderContextDescriptor contextDesc;
        contextDesc.backend = Backend::Metal;
        contextDesc.width = static_cast<uint32_t>(_width);
        contextDesc.height = static_cast<uint32_t>(_height);
        contextDesc.windowHandle = (__bridge void*)metalLayer;
        contextDesc.vsync = true;
        
        _context = LRRenderContext::Create(contextDesc);
        if (!_context) {
            NSLog(@"Failed to create Metal render context");
            return NO;
        }
        
        NSLog(@"Metal render context created successfully");
        
        // 创建顶点着色器
        ShaderDescriptor vsDesc;
        vsDesc.stage = ShaderStage::Vertex;
        vsDesc.language = ShaderLanguage::MSL;
        vsDesc.source = metalShaderSource;
        vsDesc.entryPoint = "vertexMain";
        vsDesc.debugName = "CubeVS";
        
        _vertexShader = _context->CreateShader(vsDesc);
        if (!_vertexShader || !_vertexShader->IsCompiled()) {
            NSLog(@"Failed to compile vertex shader");
            if (_vertexShader) {
                NSLog(@"Error: %s", _vertexShader->GetCompileError());
            }
            return NO;
        }
        
        // 创建片段着色器
        ShaderDescriptor fsDesc;
        fsDesc.stage = ShaderStage::Fragment;
        fsDesc.language = ShaderLanguage::MSL;
        fsDesc.source = metalShaderSource;
        fsDesc.entryPoint = "fragmentMain";
        fsDesc.debugName = "CubeFS";
        
        _fragmentShader = _context->CreateShader(fsDesc);
        if (!_fragmentShader || !_fragmentShader->IsCompiled()) {
            NSLog(@"Failed to compile fragment shader");
            return NO;
        }
        
        // 创建着色器程序
        _shaderProgram = _context->CreateShaderProgram(_vertexShader, _fragmentShader);
        if (!_shaderProgram || !_shaderProgram->IsLinked()) {
            NSLog(@"Failed to link shader program");
            return NO;
        }
        
        NSLog(@"Shaders compiled and linked successfully");
        
        // 创建顶点缓冲区
        BufferDescriptor vbDesc;
        vbDesc.size = sizeof(cubeVertices);
        vbDesc.usage = BufferUsage::Static;
        vbDesc.type = BufferType::Vertex;
        vbDesc.data = cubeVertices;
        vbDesc.stride = sizeof(Vertex);
        vbDesc.debugName = "CubeVB";
        
        _vertexBuffer = _context->CreateVertexBuffer(vbDesc);
        if (!_vertexBuffer) {
            NSLog(@"Failed to create vertex buffer");
            return NO;
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
        
        _vertexBuffer->SetVertexLayout(layout);
        
        NSLog(@"Vertex buffer created");
        
        // 使用iOS原生方式加载纹理图片
        NSString* imagePath = [[NSBundle mainBundle] pathForResource:@"xiaowa" ofType:@"png" inDirectory:@"resources"];
        if (!imagePath) {
            NSLog(@"Warning: Texture file 'resources/xiaowa.png' not found in bundle");
        }
        
        int texWidth = 0, texHeight = 0;
        unsigned char* texData = nullptr;
        
        if (imagePath) {
            // 使用CGImageSource加载图片
            NSURL* imageURL = [NSURL fileURLWithPath:imagePath];
            CGImageSourceRef imageSource = CGImageSourceCreateWithURL((__bridge CFURLRef)imageURL, NULL);
            
            if (imageSource) {
                CGImageRef cgImage = CGImageSourceCreateImageAtIndex(imageSource, 0, NULL);
                if (cgImage) {
                    texWidth = (int)CGImageGetWidth(cgImage);
                    texHeight = (int)CGImageGetHeight(cgImage);
                    
                    NSLog(@"Loading texture from: %@", imagePath);
                    NSLog(@"Image size: %dx%d", texWidth, texHeight);
                    
                    // 创建颜色空间和上下文
                    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
                    texData = (unsigned char*)malloc(texWidth * texHeight * 4);
                    
                    CGContextRef context = CGBitmapContextCreate(texData,
                                                                 texWidth,
                                                                 texHeight,
                                                                 8,
                                                                 texWidth * 4,
                                                                 colorSpace,
                                                                 kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
                    
                    if (context) {
                        // 翻转Y轴（因为CGImage和OpenGL/Metal的坐标系不同）
                        CGContextTranslateCTM(context, 0, texHeight);
                        CGContextScaleCTM(context, 1.0, -1.0);
                        
                        // 绘制图片到上下文
                        CGContextDrawImage(context, CGRectMake(0, 0, texWidth, texHeight), cgImage);
                        CGContextRelease(context);
                        
                        NSLog(@"Texture loaded successfully: %dx%d (RGBA)", texWidth, texHeight);
                    } else {
                        free(texData);
                        texData = nullptr;
                        NSLog(@"Failed to create bitmap context");
                    }
                    
                    CGColorSpaceRelease(colorSpace);
                    CGImageRelease(cgImage);
                } else {
                    NSLog(@"Failed to create CGImage from image source");
                }
                CFRelease(imageSource);
            } else {
                NSLog(@"Failed to create image source from path: %@", imagePath);
            }
        }
        
        // 如果加载失败，使用程序化棋盘纹理
        if (!texData) {
            NSLog(@"Falling back to procedural checkerboard texture...");
            texWidth = 256;
            texHeight = 256;
            texData = (unsigned char*)malloc(texWidth * texHeight * 4);
            
            for (int y = 0; y < texHeight; y++) {
                for (int x = 0; x < texWidth; x++) {
                    int checker = ((x / 32) + (y / 32)) % 2;
                    unsigned char color = checker ? 255 : 64;
                    
                    int idx = (y * texWidth + x) * 4;
                    texData[idx + 0] = color;      // R
                    texData[idx + 1] = color;      // G
                    texData[idx + 2] = color;      // B
                    texData[idx + 3] = 255;        // A
                }
            }
        }
        
        // 创建纹理
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
        
        _texture = _context->CreateTexture(texDesc);
        
        // 释放图像数据
        if (texData) {
            free(texData);
        }
        
        if (!_texture) {
            NSLog(@"Failed to create texture");
            return NO;
        }
        
        NSLog(@"Texture created successfully");
        
        // 创建管线状态
        PipelineStateDescriptor pipelineDesc;
        pipelineDesc.vertexShader = _vertexShader;
        pipelineDesc.fragmentShader = _fragmentShader;
        pipelineDesc.vertexLayout = layout;
        pipelineDesc.primitiveType = PrimitiveType::Triangles;
        
        // 启用深度测试
        pipelineDesc.depthStencilState.depthTestEnabled = true;
        pipelineDesc.depthStencilState.depthWriteEnabled = true;
        pipelineDesc.depthStencilState.depthCompareFunc = CompareFunc::Less;
        
        // 背面剔除
        pipelineDesc.rasterizerState.cullMode = CullMode::Back;
        pipelineDesc.rasterizerState.frontFace = FrontFace::CCW;
        
        pipelineDesc.debugName = "CubePipeline";
        
        _pipelineState = _context->CreatePipelineState(pipelineDesc);
        if (!_pipelineState) {
            NSLog(@"Failed to create pipeline state");
            return NO;
        }
        
        NSLog(@"Pipeline state created successfully");
        
        // 创建Uniform缓冲区
        BufferDescriptor uniformDesc;
        uniformDesc.size = sizeof(Uniforms);
        uniformDesc.usage = BufferUsage::Dynamic;
        uniformDesc.type = BufferType::Uniform;
        uniformDesc.debugName = "UniformBuffer";
        
        _uniformBuffer = _context->CreateUniformBuffer(uniformDesc);
        if (!_uniformBuffer) {
            NSLog(@"Failed to create uniform buffer");
            return NO;
        }
        
        NSLog(@"LREngine setup completed successfully");
        return YES;
    }
}

#if TARGET_OS_IPHONE
#pragma mark - OpenGL ES Setup

- (BOOL)setupLREngineWithEAGLLayer:(CAEAGLLayer *)eaglLayer {
    @autoreleasepool {
        // 设置错误回调
        LRError::SetErrorCallback([](const ErrorInfo& error) {
            NSLog(@"[LREngine GLES Error] %s (Code: %d) at %s:%d",
                  error.message.c_str(), static_cast<int>(error.code), 
                  error.file.c_str(), error.line);
        });
        
        // 创建EAGL上下文
        _eaglContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
        if (!_eaglContext) {
            NSLog(@"Failed to create EAGL context");
            return NO;
        }
        
        if (![EAGLContext setCurrentContext:_eaglContext]) {
            NSLog(@"Failed to set current EAGL context");
            return NO;
        }
        
        // 配置EAGLLayer
        eaglLayer.opaque = YES;
        eaglLayer.drawableProperties = @{
            kEAGLDrawablePropertyRetainedBacking: @NO,
            kEAGLDrawablePropertyColorFormat: kEAGLColorFormatRGBA8
        };
        
        // 创建帧缓冲区
        glGenFramebuffers(1, &_framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
        
        // 创建颜色渲染缓冲区
        glGenRenderbuffers(1, &_colorRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, _colorRenderbuffer);
        [_eaglContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:eaglLayer];
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _colorRenderbuffer);
        
        // 获取实际尺寸
        GLint backingWidth, backingHeight;
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &backingWidth);
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backingHeight);
        _width = backingWidth;
        _height = backingHeight;
        
        // 创建深度渲染缓冲区
        glGenRenderbuffers(1, &_depthRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, _depthRenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, backingWidth, backingHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthRenderbuffer);
        
        // 检查帧缓冲区完整性
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            NSLog(@"Failed to create complete framebuffer: %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
            return NO;
        }
        
        NSLog(@"OpenGL ES context and framebuffer created successfully (%dx%d)", backingWidth, backingHeight);
        
        // 创建渲染上下文
        RenderContextDescriptor contextDesc;
        contextDesc.backend = Backend::OpenGLES;
        contextDesc.width = static_cast<uint32_t>(_width);
        contextDesc.height = static_cast<uint32_t>(_height);
        contextDesc.windowHandle = (__bridge void*)eaglLayer;
        contextDesc.vsync = true;
        
        _context = LRRenderContext::Create(contextDesc);
        if (!_context) {
            NSLog(@"Failed to create OpenGL ES render context");
            return NO;
        }
        
        NSLog(@"OpenGL ES render context created successfully");
        
        // 创建顶点着色器
        ShaderDescriptor vsDesc;
        vsDesc.stage = ShaderStage::Vertex;
        vsDesc.language = ShaderLanguage::GLSL;
        vsDesc.source = glesVertexShaderSource;
        vsDesc.entryPoint = "main";
        vsDesc.debugName = "CubeVS_GLES";
        
        _vertexShader = _context->CreateShader(vsDesc);
        if (!_vertexShader || !_vertexShader->IsCompiled()) {
            NSLog(@"Failed to compile GLES vertex shader");
            if (_vertexShader) {
                NSLog(@"Error: %s", _vertexShader->GetCompileError());
            }
            return NO;
        }
        
        // 创建片段着色器
        ShaderDescriptor fsDesc;
        fsDesc.stage = ShaderStage::Fragment;
        fsDesc.language = ShaderLanguage::GLSL;
        fsDesc.source = glesFragmentShaderSource;
        fsDesc.entryPoint = "main";
        fsDesc.debugName = "CubeFS_GLES";
        
        _fragmentShader = _context->CreateShader(fsDesc);
        if (!_fragmentShader || !_fragmentShader->IsCompiled()) {
            NSLog(@"Failed to compile GLES fragment shader");
            return NO;
        }
        
        // 创建着色器程序
        _shaderProgram = _context->CreateShaderProgram(_vertexShader, _fragmentShader);
        if (!_shaderProgram || !_shaderProgram->IsLinked()) {
            NSLog(@"Failed to link GLES shader program");
            return NO;
        }
        
        NSLog(@"GLES Shaders compiled and linked successfully");
        
        // 创建顶点缓冲区
        BufferDescriptor vbDesc;
        vbDesc.size = sizeof(cubeVertices);
        vbDesc.usage = BufferUsage::Static;
        vbDesc.type = BufferType::Vertex;
        vbDesc.data = cubeVertices;
        vbDesc.stride = sizeof(Vertex);
        vbDesc.debugName = "CubeVB_GLES";
        
        _vertexBuffer = _context->CreateVertexBuffer(vbDesc);
        if (!_vertexBuffer) {
            NSLog(@"Failed to create GLES vertex buffer");
            return NO;
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
        
        _vertexBuffer->SetVertexLayout(layout);
        
        NSLog(@"GLES Vertex buffer created");
        
        // 加载纹理 - 使用与Metal相同的加载逻辑
        NSString* imagePath = [[NSBundle mainBundle] pathForResource:@"xiaowa" ofType:@"png" inDirectory:@"resources"];
        int texWidth = 0, texHeight = 0;
        unsigned char* texData = nullptr;
        
        if (imagePath) {
            NSURL* imageURL = [NSURL fileURLWithPath:imagePath];
            CGImageSourceRef imageSource = CGImageSourceCreateWithURL((__bridge CFURLRef)imageURL, NULL);
            
            if (imageSource) {
                CGImageRef cgImage = CGImageSourceCreateImageAtIndex(imageSource, 0, NULL);
                if (cgImage) {
                    texWidth = (int)CGImageGetWidth(cgImage);
                    texHeight = (int)CGImageGetHeight(cgImage);
                    
                    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
                    texData = (unsigned char*)malloc(texWidth * texHeight * 4);
                    
                    CGContextRef context = CGBitmapContextCreate(texData,
                                                                 texWidth,
                                                                 texHeight,
                                                                 8,
                                                                 texWidth * 4,
                                                                 colorSpace,
                                                                 kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
                    
                    if (context) {
                        CGContextTranslateCTM(context, 0, texHeight);
                        CGContextScaleCTM(context, 1.0, -1.0);
                        CGContextDrawImage(context, CGRectMake(0, 0, texWidth, texHeight), cgImage);
                        CGContextRelease(context);
                    } else {
                        free(texData);
                        texData = nullptr;
                    }
                    
                    CGColorSpaceRelease(colorSpace);
                    CGImageRelease(cgImage);
                }
                CFRelease(imageSource);
            }
        }
        
        // 如果加载失败，使用棋盘纹理
        if (!texData) {
            NSLog(@"Falling back to procedural checkerboard texture...");
            texWidth = 256;
            texHeight = 256;
            texData = (unsigned char*)malloc(texWidth * texHeight * 4);
            
            for (int y = 0; y < texHeight; y++) {
                for (int x = 0; x < texWidth; x++) {
                    int checker = ((x / 32) + (y / 32)) % 2;
                    unsigned char color = checker ? 255 : 64;
                    
                    int idx = (y * texWidth + x) * 4;
                    texData[idx + 0] = color;
                    texData[idx + 1] = color;
                    texData[idx + 2] = color;
                    texData[idx + 3] = 255;
                }
            }
        }
        
        // 创建纹理
        TextureDescriptor texDesc;
        texDesc.type = TextureType::Texture2D;
        texDesc.width = texWidth;
        texDesc.height = texHeight;
        texDesc.depth = 1;
        texDesc.format = PixelFormat::RGBA8;
        texDesc.mipLevels = 1;
        texDesc.data = texData;
        texDesc.generateMipmaps = false;
        texDesc.debugName = "CubeTexture_GLES";
        texDesc.sampler.minFilter = FilterMode::Linear;
        texDesc.sampler.magFilter = FilterMode::Linear;
        texDesc.sampler.wrapU = WrapMode::Repeat;
        texDesc.sampler.wrapV = WrapMode::Repeat;
        
        _texture = _context->CreateTexture(texDesc);
        
        if (texData) {
            free(texData);
        }
        
        if (!_texture) {
            NSLog(@"Failed to create GLES texture");
            return NO;
        }
        
        NSLog(@"GLES Texture created successfully");
        
        // 创建管线状态
        PipelineStateDescriptor pipelineDesc;
        pipelineDesc.vertexShader = _vertexShader;
        pipelineDesc.fragmentShader = _fragmentShader;
        pipelineDesc.vertexLayout = layout;
        pipelineDesc.primitiveType = PrimitiveType::Triangles;
        pipelineDesc.depthStencilState.depthTestEnabled = true;
        pipelineDesc.depthStencilState.depthWriteEnabled = true;
        pipelineDesc.depthStencilState.depthCompareFunc = CompareFunc::Less;
        pipelineDesc.rasterizerState.cullMode = CullMode::Back;
        pipelineDesc.rasterizerState.frontFace = FrontFace::CCW;
        pipelineDesc.debugName = "CubePipeline_GLES";
        
        _pipelineState = _context->CreatePipelineState(pipelineDesc);
        if (!_pipelineState) {
            NSLog(@"Failed to create GLES pipeline state");
            return NO;
        }
        
        NSLog(@"GLES Pipeline state created successfully");
        
        // 创建Uniform缓冲区
        BufferDescriptor uniformDesc;
        uniformDesc.size = sizeof(Uniforms);
        uniformDesc.usage = BufferUsage::Dynamic;
        uniformDesc.type = BufferType::Uniform;
        uniformDesc.debugName = "UniformBuffer_GLES";
        
        _uniformBuffer = _context->CreateUniformBuffer(uniformDesc);
        if (!_uniformBuffer) {
            NSLog(@"Failed to create GLES uniform buffer");
            return NO;
        }
        
        NSLog(@"LREngine GLES setup completed successfully");
        return YES;
    }
}
#endif

- (void)renderFrame:(float)deltaTime {
    if (!_context) {
        return;
    }
    
#if TARGET_OS_IPHONE
    // GLES 需要设置当前上下文
    if (_backend == LRRenderBackendOpenGLES && _eaglContext) {
        [EAGLContext setCurrentContext:_eaglContext];
        glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    }
#endif
    
    @autoreleasepool {
        // 更新旋转角度
        _rotationAngle += deltaTime;
        
        // 计算MVP矩阵
        Uniforms uniforms;
        
        // 模型矩阵 - 旋转立方体
        uniforms.modelMatrix = Mat4f::rotateY(_rotationAngle) * 
                               Mat4f::rotateX(_rotationAngle * 0.5f);
        
        // 视图矩阵 - 相机位置
        Vec3f eye(0.0f, 0.0f, 6.0f);
        Vec3f center(0.0f, 0.0f, 0.0f);
        Vec3f up(0.0f, 1.0f, 0.0f);
        uniforms.viewMatrix = Mat4f::lookAt(eye, center, up);
        
        // 投影矩阵 - 透视投影
        float aspect = static_cast<float>(_width) / static_cast<float>(_height);
        uniforms.projectionMatrix = Mat4f::perspective(45.0f * 3.14159f / 180.0f, 
                                                       aspect, 0.1f, 100.0f);
        
        // 更新Uniform缓冲区
        _uniformBuffer->UpdateData(&uniforms, sizeof(Uniforms), 0);
        
        // 开始帧
        _context->BeginFrame();
        
        _context->BeginRenderPass();
        
        // 清除屏幕（深蓝色背景）
        _context->Clear(ClearFlag_Color | ClearFlag_Depth, 
                       0.2f, 0.3f, 0.4f, 1.0f, 1.0f, 0);
        
        // 使用着色器程序
//        _shaderProgram->Use();
        
        // 设置管线状态
        _context->SetPipelineState(_pipelineState);
        
        // 绑定资源
        _context->SetVertexBuffer(_vertexBuffer, 0);
        
#if TARGET_OS_IPHONE
        if (_backend == LRRenderBackendOpenGLES) {
            // GLES 使用 Uniform 设置矩阵
            _pipelineState->GetShaderProgram()->SetUniformMatrix4("uModelMatrix", uniforms.modelMatrix);
            _pipelineState->GetShaderProgram()->SetUniformMatrix4("uViewMatrix", uniforms.viewMatrix);
            _pipelineState->GetShaderProgram()->SetUniformMatrix4("uProjectionMatrix", uniforms.projectionMatrix);
            _pipelineState->GetShaderProgram()->SetUniform("uTexture", 0);
        } else {
            _context->SetUniformBuffer(_uniformBuffer, 1);
        }
#else
        _context->SetUniformBuffer(_uniformBuffer, 1);
#endif
        
        _context->SetTexture(_texture, 0);
        
        // 绘制立方体
        _context->Draw(0, 36);
        
        _context->EndRenderPass();
        
        // 结束帧并呈现
        _context->EndFrame();
        
#if TARGET_OS_IPHONE
        // GLES 需要显式呈现
        if (_backend == LRRenderBackendOpenGLES && _eaglContext) {
            glBindRenderbuffer(GL_RENDERBUFFER, _colorRenderbuffer);
            [_eaglContext presentRenderbuffer:GL_RENDERBUFFER];
        }
#endif
    }
}

- (void)resizeWithSize:(CGSize)newSize {
    _width = (NSInteger)newSize.width;
    _height = (NSInteger)newSize.height;
    
    if (_context) {
        _context->SetViewport(0, 0, (int32_t)_width, (int32_t)_height);
    }
    
#if TARGET_OS_IPHONE
    // GLES 需要重新创建渲染缓冲区
    if (_backend == LRRenderBackendOpenGLES && _eaglContext && _eaglLayer) {
        [EAGLContext setCurrentContext:_eaglContext];
        
        // 删除旧的渲染缓冲区
        if (_colorRenderbuffer) {
            glDeleteRenderbuffers(1, &_colorRenderbuffer);
            _colorRenderbuffer = 0;
        }
        if (_depthRenderbuffer) {
            glDeleteRenderbuffers(1, &_depthRenderbuffer);
            _depthRenderbuffer = 0;
        }
        
        // 重新创建颜色渲染缓冲区
        glGenRenderbuffers(1, &_colorRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, _colorRenderbuffer);
        [_eaglContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:_eaglLayer];
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _colorRenderbuffer);
        
        // 获取新尺寸
        GLint backingWidth, backingHeight;
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &backingWidth);
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backingHeight);
        _width = backingWidth;
        _height = backingHeight;
        
        // 重新创建深度渲染缓冲区
        glGenRenderbuffers(1, &_depthRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, _depthRenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, backingWidth, backingHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthRenderbuffer);
        
        _context->SetViewport(0, 0, backingWidth, backingHeight);
    }
#endif
}

- (void)cleanup {
#if TARGET_OS_IPHONE
    // 清理 GLES 资源
    if (_backend == LRRenderBackendOpenGLES) {
        if (_eaglContext) {
            [EAGLContext setCurrentContext:_eaglContext];
            
            if (_framebuffer) {
                glDeleteFramebuffers(1, &_framebuffer);
                _framebuffer = 0;
            }
            if (_colorRenderbuffer) {
                glDeleteRenderbuffers(1, &_colorRenderbuffer);
                _colorRenderbuffer = 0;
            }
            if (_depthRenderbuffer) {
                glDeleteRenderbuffers(1, &_depthRenderbuffer);
                _depthRenderbuffer = 0;
            }
            
            [EAGLContext setCurrentContext:nil];
            _eaglContext = nil;
        }
        _eaglLayer = nil;
    }
#endif
    
    if (_texture) {
        delete _texture;
        _texture = nullptr;
    }
    
    if (_uniformBuffer) {
        delete _uniformBuffer;
        _uniformBuffer = nullptr;
    }
    
    if (_pipelineState) {
        delete _pipelineState;
        _pipelineState = nullptr;
    }
    
    if (_vertexBuffer) {
        delete _vertexBuffer;
        _vertexBuffer = nullptr;
    }
    
    if (_shaderProgram) {
        delete _shaderProgram;
        _shaderProgram = nullptr;
    }
    
    if (_fragmentShader) {
        delete _fragmentShader;
        _fragmentShader = nullptr;
    }
    
    if (_vertexShader) {
        delete _vertexShader;
        _vertexShader = nullptr;
    }
    
    if (_context) {
        LRRenderContext::Destroy(_context);
        _context = nullptr;
    }
    
    NSLog(@"LREngine resources cleaned up");
}

- (void)dealloc {
    [self cleanup];
}

@end
