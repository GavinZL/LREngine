//
//  ViewController.swift
//  LREngine
//
//  Created by BIGO on 2026/1/17.
//  使用LREngine Metal后端渲染3D立方体
//

import UIKit
import MetalKit
import QuartzCore

class ViewController: UIViewController {
    
    // MARK: - Properties
    
    /// Metal视图
    private var metalView: UIView!
    
    /// CAMetalLayer用于Metal渲染
    private var metalLayer: CAMetalLayer!
    
    /// LREngine渲染器
    private var renderer: LREngineRenderer?
    
    /// 显示链接用于驱动渲染循环
    private var displayLink: CADisplayLink?
    
    /// 上一帧的时间戳
    private var lastFrameTime: CFTimeInterval = 0

    // MARK: - Lifecycle
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        // 设置Metal视图
        setupMetalView()
        
        // 初始化LREngine渲染器
        setupRenderer()
        
        // 启动渲染循环
        startRenderLoop()
    }
    
    override func viewDidDisappear(_ animated: Bool) {
        super.viewDidDisappear(animated)
        
        // 停止渲染循环
        stopRenderLoop()
    }
    
    deinit {
        // 清理渲染器资源
        renderer?.cleanup()
    }
    
    // MARK: - Setup
    
    private func setupMetalView() {
        // 创建全屏Metal视图
        metalView = UIView(frame: view.bounds)
        metalView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
        view.addSubview(metalView)
        
        // 创建CAMetalLayer
        metalLayer = CAMetalLayer()
        metalLayer.device = MTLCreateSystemDefaultDevice()
        metalLayer.pixelFormat = .bgra8Unorm
        metalLayer.framebufferOnly = false
        metalLayer.frame = metalView.bounds
        metalView.layer.addSublayer(metalLayer)
        
        print("Metal view and layer setup completed")
    }
    
    private func setupRenderer() {
        guard let metalLayer = metalLayer else {
            print("Error: Metal layer is nil")
            return
        }
        
        // 获取屏幕的原生分辨率（支持Retina显示）
        let scale = UIScreen.main.nativeScale
        let width = Int(view.bounds.width * scale)
        let height = Int(view.bounds.height * scale)
        
        // 设置Metal layer的可绘制尺寸
        metalLayer.drawableSize = CGSize(width: width, height: height)
        
        // 初始化LREngine渲染器
        renderer = LREngineRenderer(metalLayer: metalLayer, 
                                    width: width, 
                                    height: height)
        
        if renderer == nil {
            print("Error: Failed to create LREngine renderer")
            return
        }
        
        print("LREngine renderer initialized successfully")
        print("Rendering resolution: \(width) x \(height)")
    }
    
    // MARK: - Render Loop
    
    private func startRenderLoop() {
        // 创建显示链接
        displayLink = CADisplayLink(target: self, selector: #selector(renderFrame))
        displayLink?.add(to: .main, forMode: .default)
        
        // 记录初始时间
        lastFrameTime = CACurrentMediaTime()
        
        print("Render loop started")
    }
    
    private func stopRenderLoop() {
        displayLink?.invalidate()
        displayLink = nil
        
        print("Render loop stopped")
    }
    
    @objc private func renderFrame() {
        // 计算帧间隔时间
        let currentTime = CACurrentMediaTime()
        let deltaTime = Float(currentTime - lastFrameTime)
        lastFrameTime = currentTime
        
        // 调用LREngine渲染器绘制一帧
        renderer?.renderFrame(deltaTime)
    }
    
    // MARK: - Layout
    
    override func viewDidLayoutSubviews() {
        super.viewDidLayoutSubviews()
        
        // 更新Metal layer尺寸
        metalLayer?.frame = metalView.bounds
        
        // 获取新的渲染尺寸
        let scale = UIScreen.main.nativeScale
        let newSize = CGSize(width: view.bounds.width * scale, 
                           height: view.bounds.height * scale)
        
        // 更新Metal layer的可绘制尺寸
        metalLayer?.drawableSize = newSize
        
        // 通知渲染器尺寸变化
        renderer?.resize(with: newSize)
    }
    
    // MARK: - Status Bar
    
    override var prefersStatusBarHidden: Bool {
        return true // 隐藏状态栏以获得更好的显示效果
    }
}
