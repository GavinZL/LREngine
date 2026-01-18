package com.example.lrenginedemo

import android.os.Bundle
import android.util.Log
import android.view.ViewGroup
import android.view.WindowManager
import androidx.activity.ComponentActivity
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat

/**
 * LREngine Android Demo主活动
 * 
 * 展示使用LREngine渲染引擎在Android平台上渲染旋转的3D立方体
 */
class MainActivity : ComponentActivity() {
    
    companion object {
        private const val TAG = "MainActivity"
    }
    
    // LREngine渲染视图
    private var renderView: LREngineSurfaceView? = null
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        Log.i(TAG, "onCreate")
        
        // 设置全屏沉浸式模式
        setupImmersiveMode()
        
        // 创建并设置LREngine渲染视图
        Log.i(TAG, "Creating LREngineSurfaceView")
        renderView = LREngineSurfaceView(this)
        
        // 设置布局参数为MATCH_PARENT，确保SurfaceView填满整个屏幕
        val layoutParams = ViewGroup.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.MATCH_PARENT
        )
        setContentView(renderView, layoutParams)
        Log.i(TAG, "LREngineSurfaceView set as content view with MATCH_PARENT")
    }
    
    /**
     * 设置全屏沉浸式模式
     */
    private fun setupImmersiveMode() {
        // 保持屏幕常亮
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        
        // 设置全屏
        WindowCompat.setDecorFitsSystemWindows(window, false)
        
        // 隐藏系统栏
        val windowInsetsController = WindowInsetsControllerCompat(window, window.decorView)
        windowInsetsController.apply {
            hide(WindowInsetsCompat.Type.systemBars())
            systemBarsBehavior = WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        }
    }
    
    override fun onResume() {
        super.onResume()
        Log.i(TAG, "onResume")
        renderView?.resume()
    }
    
    override fun onPause() {
        super.onPause()
        Log.i(TAG, "onPause")
        renderView?.pause()
    }
    
    override fun onDestroy() {
        super.onDestroy()
        Log.i(TAG, "onDestroy")
        renderView?.release()
        renderView = null
    }
}