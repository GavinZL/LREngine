#!/bin/bash

################################################################################
# LREngine iOS 构建验证脚本
# 
# 功能：验证iOS构建的输出是否正确
################################################################################

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
OUTPUT_DIR="${PROJECT_ROOT}/build/ios"

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[✓]${NC} $1"
}

print_error() {
    echo -e "${RED}[✗]${NC} $1"
}

print_header() {
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}$1${NC}"
    echo -e "${GREEN}========================================${NC}"
}

verify_static_library() {
    print_header "验证静态库"
    
    local lib_file="${OUTPUT_DIR}/lib/liblrengine.a"
    
    if [[ ! -f "$lib_file" ]]; then
        print_error "静态库不存在: $lib_file"
        return 1
    fi
    print_success "静态库存在"
    
    # 检查架构
    print_info "检查架构..."
    local archs=$(lipo -info "$lib_file" 2>/dev/null | grep -o "arm64")
    if [[ -n "$archs" ]]; then
        print_success "包含arm64架构"
    else
        print_error "缺少arm64架构"
        return 1
    fi
    
    # 检查符号
    print_info "检查关键符号..."
    if nm "$lib_file" 2>/dev/null | grep -q "LRDeviceFactory"; then
        print_success "包含LRDeviceFactory符号"
    else
        print_error "缺少关键符号"
        return 1
    fi
    
    # 显示库大小
    local size=$(du -h "$lib_file" | awk '{print $1}')
    print_info "静态库大小: $size"
    
    return 0
}

verify_framework() {
    print_header "验证Framework"
    
    local framework_dir="${OUTPUT_DIR}/framework/LREngine.framework"
    
    if [[ ! -d "$framework_dir" ]]; then
        print_error "Framework不存在: $framework_dir"
        return 1
    fi
    print_success "Framework目录存在"
    
    # 检查二进制文件
    local binary="${framework_dir}/LREngine"
    if [[ ! -f "$binary" ]]; then
        print_error "Framework二进制文件不存在"
        return 1
    fi
    print_success "Framework二进制文件存在"
    
    # 检查架构
    print_info "检查架构..."
    local archs=$(lipo -info "$binary" 2>/dev/null | grep -o "arm64")
    if [[ -n "$archs" ]]; then
        print_success "包含arm64架构"
    else
        print_error "缺少arm64架构"
        return 1
    fi
    
    # 检查头文件
    if [[ ! -d "${framework_dir}/Headers" ]]; then
        print_error "Headers目录不存在"
        return 1
    fi
    print_success "Headers目录存在"
    
    # 检查关键头文件
    local key_headers=(
        "LREngine.h"
        "core/LRRenderContext.h"
        "factory/LRDeviceFactory.h"
    )
    
    for header in "${key_headers[@]}"; do
        if [[ ! -f "${framework_dir}/Headers/${header}" ]]; then
            print_error "缺少头文件: ${header}"
            return 1
        fi
    done
    print_success "关键头文件完整"
    
    # 检查Info.plist
    if [[ ! -f "${framework_dir}/Info.plist" ]]; then
        print_error "Info.plist不存在"
        return 1
    fi
    print_success "Info.plist存在"
    
    # 显示Framework大小
    local size=$(du -sh "$framework_dir" | awk '{print $1}')
    print_info "Framework大小: $size"
    
    return 0
}

verify_headers() {
    print_header "验证头文件"
    
    local include_dir="${OUTPUT_DIR}/include/lrengine"
    
    if [[ ! -d "$include_dir" ]]; then
        print_error "头文件目录不存在: $include_dir"
        return 1
    fi
    print_success "头文件目录存在"
    
    # 检查关键头文件
    local key_headers=(
        "core/LRRenderContext.h"
        "core/LRBuffer.h"
        "core/LRShader.h"
        "core/LRTexture.h"
        "factory/LRDeviceFactory.h"
        "math/Vec3.hpp"
        "math/Mat4.hpp"
    )
    
    for header in "${key_headers[@]}"; do
        if [[ ! -f "${include_dir}/${header}" ]]; then
            print_error "缺少头文件: ${header}"
            return 1
        fi
    done
    print_success "所有关键头文件完整"
    
    return 0
}

main() {
    print_header "LREngine iOS 构建验证"
    
    if [[ ! -d "$OUTPUT_DIR" ]]; then
        print_error "输出目录不存在: $OUTPUT_DIR"
        echo "请先运行构建脚本: ./script/build_ios.sh"
        exit 1
    fi
    
    local failed=0
    
    # 验证静态库
    if [[ -d "${OUTPUT_DIR}/lib" ]]; then
        verify_static_library || ((failed++))
    else
        print_info "跳过静态库验证（未构建）"
    fi
    
    # 验证Framework
    if [[ -d "${OUTPUT_DIR}/framework" ]]; then
        verify_framework || ((failed++))
    else
        print_info "跳过Framework验证（未构建）"
    fi
    
    # 验证头文件
    if [[ -d "${OUTPUT_DIR}/include" ]]; then
        verify_headers || ((failed++))
    else
        print_info "跳过头文件验证（未构建）"
    fi
    
    # 总结
    print_header "验证结果"
    
    if [[ $failed -eq 0 ]]; then
        print_success "所有验证通过！iOS构建正常"
        exit 0
    else
        print_error "有 $failed 项验证失败"
        exit 1
    fi
}

main
