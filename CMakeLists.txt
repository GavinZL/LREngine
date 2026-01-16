cmake_minimum_required(VERSION 3.15)
project(LREngine VERSION 1.0.0 LANGUAGES CXX)

# C++17标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编辑器跳转
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 选项
option(LRENGINE_BUILD_EXAMPLES "Build examples" ON)
option(LRENGINE_BUILD_TESTS "Build tests" OFF)
option(LRENGINE_ENABLE_OPENGL "Enable OpenGL backend" ON)
option(LRENGINE_ENABLE_OPENGLES "Enable OpenGL ES backend" OFF)
option(LRENGINE_ENABLE_METAL "Enable Metal backend" OFF)
option(LRENGINE_ENABLE_VULKAN "Enable Vulkan backend" OFF)

# 平台检测
if(APPLE)
    set(LRENGINE_PLATFORM_APPLE TRUE)
    if(IOS)
        set(LRENGINE_PLATFORM_IOS TRUE)
    else()
        set(LRENGINE_PLATFORM_MACOS TRUE)
    endif()
elseif(WIN32)
    set(LRENGINE_PLATFORM_WINDOWS TRUE)
elseif(UNIX)
    set(LRENGINE_PLATFORM_LINUX TRUE)
endif()

# 查找OpenGL
if(LRENGINE_ENABLE_OPENGL)
    find_package(OpenGL REQUIRED)
endif()

# 查找GLFW（用于示例）
if(LRENGINE_BUILD_EXAMPLES)
    find_package(glfw3 QUIET)
    if(NOT glfw3_FOUND)
        message(STATUS "GLFW not found, examples will not be built")
        set(LRENGINE_BUILD_EXAMPLES OFF)
    endif()
endif()

# 头文件目录
set(LRENGINE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 核心源文件
set(LRENGINE_CORE_SOURCES
    src/core/LRError.cpp
    src/core/LRResource.cpp
    src/core/LRBuffer.cpp
    src/core/LRShader.cpp
    src/core/LRTexture.cpp
    src/core/LRFrameBuffer.cpp
    src/core/LRPipelineState.cpp
    src/core/LRFence.cpp
    src/core/LRRenderContext.cpp
)

# 工具库源文件
set(LRENGINE_UTILS_SOURCES
    src/utils/LRLog.cpp
)

# 核心头文件
set(LRENGINE_CORE_HEADERS
    include/lrengine/core/LRDefines.h
    include/lrengine/core/LRTypes.h
    include/lrengine/core/LRError.h
    include/lrengine/core/LRResource.h
    include/lrengine/core/LRBuffer.h
    include/lrengine/core/LRShader.h
    include/lrengine/core/LRTexture.h
    include/lrengine/core/LRFrameBuffer.h
    include/lrengine/core/LRPipelineState.h
    include/lrengine/core/LRFence.h
    include/lrengine/core/LRRenderContext.h
)

# 工具库头文件
set(LRENGINE_UTILS_HEADERS
    include/lrengine/utils/LRLog.h
)

# 平台接口头文件
set(LRENGINE_INTERFACE_HEADERS
    src/platform/interface/IBufferImpl.h
    src/platform/interface/IShaderImpl.h
    src/platform/interface/ITextureImpl.h
    src/platform/interface/IFrameBufferImpl.h
    src/platform/interface/IPipelineStateImpl.h
    src/platform/interface/IFenceImpl.h
    src/platform/interface/IRenderContextImpl.h
)

# OpenGL后端源文件
if(LRENGINE_ENABLE_OPENGL)
    set(LRENGINE_OPENGL_SOURCES
        src/platform/opengl/ContextGL.cpp
        src/platform/opengl/BufferGL.cpp
        src/platform/opengl/ShaderGL.cpp
        src/platform/opengl/TextureGL.cpp
        src/platform/opengl/FrameBufferGL.cpp
        src/platform/opengl/PipelineStateGL.cpp
        src/platform/opengl/FenceGL.cpp
        src/platform/opengl/StateCacheGL.cpp
        src/platform/opengl/TypeConverterGL.cpp
        src/platform/opengl/DeviceFactoryGL.cpp
    )
    
    set(LRENGINE_OPENGL_HEADERS
        src/platform/opengl/ContextGL.h
        src/platform/opengl/BufferGL.h
        src/platform/opengl/ShaderGL.h
        src/platform/opengl/TextureGL.h
        src/platform/opengl/FrameBufferGL.h
        src/platform/opengl/PipelineStateGL.h
        src/platform/opengl/FenceGL.h
        src/platform/opengl/StateCacheGL.h
        src/platform/opengl/TypeConverterGL.h
        src/platform/opengl/DeviceFactoryGL.h
    )
endif()

# 工厂源文件
set(LRENGINE_FACTORY_SOURCES
    src/factory/LRDeviceFactory.cpp
)

set(LRENGINE_FACTORY_HEADERS
    include/lrengine/factory/LRDeviceFactory.h
)

# 数学库头文件
set(LRENGINE_MATH_HEADERS
    include/lrengine/math/MathFwd.hpp
    include/lrengine/math/MathDef.hpp
    include/lrengine/math/Vec2.hpp
    include/lrengine/math/Vec3.hpp
    include/lrengine/math/Vec4.hpp
    include/lrengine/math/Mat3.hpp
    include/lrengine/math/Mat4.hpp
    include/lrengine/math/Quaternion.hpp
)

# 合并所有源文件
set(LRENGINE_SOURCES
    ${LRENGINE_CORE_SOURCES}
    ${LRENGINE_UTILS_SOURCES}
    ${LRENGINE_FACTORY_SOURCES}
)

if(LRENGINE_ENABLE_OPENGL)
    list(APPEND LRENGINE_SOURCES ${LRENGINE_OPENGL_SOURCES})
endif()

# 合并所有头文件
set(LRENGINE_HEADERS
    ${LRENGINE_CORE_HEADERS}
    ${LRENGINE_UTILS_HEADERS}
    ${LRENGINE_INTERFACE_HEADERS}
    ${LRENGINE_FACTORY_HEADERS}
    ${LRENGINE_MATH_HEADERS}
)

if(LRENGINE_ENABLE_OPENGL)
    list(APPEND LRENGINE_HEADERS ${LRENGINE_OPENGL_HEADERS})
endif()

# 创建库
add_library(lrengine STATIC ${LRENGINE_SOURCES} ${LRENGINE_HEADERS})

# 包含目录
target_include_directories(lrengine
    PUBLIC
        $<BUILD_INTERFACE:${LRENGINE_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 链接库
if(LRENGINE_ENABLE_OPENGL)
    target_link_libraries(lrengine PUBLIC OpenGL::GL)
    target_compile_definitions(lrengine PUBLIC LRENGINE_ENABLE_OPENGL)
    
    # macOS需要额外链接
    if(APPLE)
        target_link_libraries(lrengine PUBLIC "-framework Cocoa" "-framework IOKit")
    endif()
endif()

# 编译定义
target_compile_definitions(lrengine PRIVATE LRENGINE_EXPORT)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(lrengine PUBLIC LR_DEBUG)
endif()

# 警告级别
if(MSVC)
    target_compile_options(lrengine PRIVATE /W4)
else()
    target_compile_options(lrengine PRIVATE -Wall -Wextra -Wpedantic)
endif()

# 示例
if(LRENGINE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# 测试
if(LRENGINE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 安装规则
install(TARGETS lrengine
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/lrengine DESTINATION include)

# 打印配置摘要
message(STATUS "")
message(STATUS "LREngine Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  OpenGL: ${LRENGINE_ENABLE_OPENGL}")
message(STATUS "  OpenGL ES: ${LRENGINE_ENABLE_OPENGLES}")
message(STATUS "  Metal: ${LRENGINE_ENABLE_METAL}")
message(STATUS "  Vulkan: ${LRENGINE_ENABLE_VULKAN}")
message(STATUS "  Examples: ${LRENGINE_BUILD_EXAMPLES}")
message(STATUS "  Tests: ${LRENGINE_BUILD_TESTS}")
message(STATUS "")
