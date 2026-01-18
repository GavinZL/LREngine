package com.example.lrenginedemo

import android.util.Log
import android.view.Surface

/**
 * LREngine渲染器类
 * 
 * 提供JNI原生方法调用接口，负责与C++层的LREngine进行交互
 */
class LREngineRenderer {
    
    companion object {
        private const val TAG = "LREngineRenderer"
        
        // 加载原生库
        init {
            try {
                Log.i(TAG, "Loading native libraries...")
                // 加载LREngine库（libc++_shared.so会由系统自动加载）
                System.loadLibrary("lrengine")
                Log.i(TAG, "liblrengine.so loaded successfully")
                // 加载JNI桥接库
                System.loadLibrary("native-lib")
                Log.i(TAG, "libnative-lib.so loaded successfully")
            } catch (e: UnsatisfiedLinkError) {
                Log.e(TAG, "Failed to load native library: ${e.message}")
                e.printStackTrace()
            }
        }
    }
    
    /**
     * 初始化渲染器
     * @param surface Android Surface对象
     * @param width 渲染宽度
     * @param height 渲染高度
     * @return 是否初始化成功
     */
    external fun nativeInit(surface: Surface, width: Int, height: Int): Boolean
    
    /**
     * 调整视口大小
     * @param width 新宽度
     * @param height 新高度
     */
    external fun nativeResize(width: Int, height: Int)
    
    /**
     * 渲染一帧
     */
    external fun nativeRender()
    
    /**
     * 销毁渲染器
     */
    external fun nativeDestroy()
}
