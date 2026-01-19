//
//  LREngineBridge.h
//  LREngine
//
//  Created on 2026/1/17.
//  Objective-C++ 桥接层 - 封装LREngine C++接口供Swift调用
//

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

#define TARGET_OS_IPHONE 1
#if TARGET_OS_IPHONE
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES3/gl.h>
#import <OpenGLES/ES3/glext.h>
#endif

NS_ASSUME_NONNULL_BEGIN

/**
 * @brief 渲染后端类型
 */
typedef NS_ENUM(NSInteger, LRRenderBackend) {
    LRRenderBackendMetal = 0,
    LRRenderBackendOpenGLES = 1
};

/**
 * @brief LREngine渲染器的Objective-C封装类
 * 
 * 提供Swift可调用的接口来使用LREngine的Metal/OpenGL ES渲染功能
 */
@interface LREngineRenderer : NSObject

/** 当前使用的渲染后端 */
@property (nonatomic, readonly) LRRenderBackend backend;

/**
 * @brief 使用Metal后端初始化渲染器
 * @param metalLayer CAMetalLayer用于Metal渲染
 * @param width 渲染宽度
 * @param height 渲染高度
 * @return 初始化是否成功
 */
- (instancetype)initWithMetalLayer:(CAMetalLayer *)metalLayer
                             width:(NSInteger)width
                            height:(NSInteger)height;

#if TARGET_OS_IPHONE
/**
 * @brief 使用OpenGL ES后端初始化渲染器
 * @param eaglLayer CAEAGLLayer用于OpenGL ES渲染
 * @param width 渲染宽度
 * @param height 渲染高度
 * @return 初始化是否成功
 */
- (instancetype)initWithEAGLLayer:(CAEAGLLayer *)eaglLayer
                            width:(NSInteger)width
                           height:(NSInteger)height;
#endif

/**
 * @brief 渲染一帧
 * @param deltaTime 帧间隔时间（秒）
 */
- (void)renderFrame:(float)deltaTime;

/**
 * @brief 清理资源
 */
- (void)cleanup;

/**
 * @brief 处理视图大小变化
 * @param newSize 新的视图尺寸
 */
- (void)resizeWithSize:(CGSize)newSize;

@end

NS_ASSUME_NONNULL_END
