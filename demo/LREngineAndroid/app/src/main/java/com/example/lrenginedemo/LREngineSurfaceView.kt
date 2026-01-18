package com.example.lrenginedemo

import android.content.Context
import android.util.AttributeSet
import android.util.Log
import android.view.Choreographer
import android.view.SurfaceHolder
import android.view.SurfaceView

/**
 * LREngine渲染Surface视图
 * 
 * 基于SurfaceView实现，提供OpenGL ES 3.0渲染支持
 * 使用Choreographer进行帧同步，确保平滑渲染
 */
class LREngineSurfaceView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null
) : SurfaceView(context, attrs), SurfaceHolder.Callback, Choreographer.FrameCallback {
    
    companion object {
        private const val TAG = "LREngineSurfaceView"
    }
    
    // 渲染器实例
    private val renderer = LREngineRenderer()
    
    // 渲染状态
    private var isRendering = false
    private var isSurfaceCreated = false
    
    // 帧同步
    private val choreographer = Choreographer.getInstance()
    
    // Surface尺寸
    private var surfaceWidth = 0
    private var surfaceHeight = 0
    
    init {
        Log.i(TAG, "init: setting up SurfaceView")
        
        // 设置Surface回调
        holder.addCallback(this)
        Log.i(TAG, "init: callback added to holder")
        
        // 设置Z轴顺序（确保在顶部渲染）
        setZOrderOnTop(false)
        
        // 确保surface类型正确
        holder.setFormat(android.graphics.PixelFormat.RGBA_8888)
        Log.i(TAG, "init: holder format set to RGBA_8888")
    }
    
    /**
     * Surface创建回调
     */
    override fun surfaceCreated(holder: SurfaceHolder) {
        Log.i(TAG, "surfaceCreated")
        isSurfaceCreated = true
    }
    
    /**
     * Surface尺寸变化回调
     */
    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        Log.i(TAG, "surfaceChanged: ${width}x${height}")
        
        surfaceWidth = width
        surfaceHeight = height
        
        // 在渲染线程中初始化或调整大小
        if (!isRendering) {
            // 首次初始化
            val surface = holder.surface
            if (surface != null && surface.isValid) {
                val success = renderer.nativeInit(surface, width, height)
                if (success) {
                    Log.i(TAG, "Renderer initialized successfully")
                    isRendering = true
                    // 启动渲染循环
                    choreographer.postFrameCallback(this)
                } else {
                    Log.e(TAG, "Failed to initialize renderer")
                }
            }
        } else {
            // 调整视口大小
            renderer.nativeResize(width, height)
        }
    }
    
    /**
     * Surface销毁回调
     */
    override fun surfaceDestroyed(holder: SurfaceHolder) {
        Log.i(TAG, "surfaceDestroyed")
        
        // 停止渲染循环
        isRendering = false
        isSurfaceCreated = false
        
        // 移除帧回调
        choreographer.removeFrameCallback(this)
        
        // 销毁渲染器
        renderer.nativeDestroy()
        
        Log.i(TAG, "Renderer destroyed")
    }
    
    /**
     * Choreographer帧回调 - 用于同步渲染
     */
    override fun doFrame(frameTimeNanos: Long) {
        if (!isRendering || !isSurfaceCreated) {
            return
        }
        
        // 渲染一帧
        renderer.nativeRender()
        
        // 请求下一帧
        choreographer.postFrameCallback(this)
    }
    
    /**
     * 暂停渲染
     */
    fun pause() {
        Log.i(TAG, "pause")
        if (isRendering) {
            choreographer.removeFrameCallback(this)
        }
    }
    
    /**
     * 恢复渲染
     */
    fun resume() {
        Log.i(TAG, "resume")
        if (isRendering && isSurfaceCreated) {
            choreographer.postFrameCallback(this)
        }
    }
    
    /**
     * 释放资源
     */
    fun release() {
        Log.i(TAG, "release")
        pause()
        if (isSurfaceCreated) {
            renderer.nativeDestroy()
            isRendering = false
        }
    }
}
