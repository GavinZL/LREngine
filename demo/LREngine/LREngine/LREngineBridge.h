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

NS_ASSUME_NONNULL_BEGIN

/**
 * @brief LREngine渲染器的Objective-C封装类
 * 
 * 提供Swift可调用的接口来使用LREngine的Metal渲染功能
 */
@interface LREngineRenderer : NSObject

/**
 * @brief 初始化渲染器
 * @param metalLayer CAMetalLayer用于Metal渲染
 * @param width 渲染宽度
 * @param height 渲染高度
 * @return 初始化是否成功
 */
- (instancetype)initWithMetalLayer:(CAMetalLayer *)metalLayer
                             width:(NSInteger)width
                            height:(NSInteger)height;

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
