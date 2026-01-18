# Examples CMakeLists.txt

# HelloTriangle示例 (OpenGL)
if(LRENGINE_ENABLE_OPENGL)
    add_executable(HelloTriangle
        HelloTriangle/main.cpp
    )

    target_link_libraries(HelloTriangle PRIVATE
        lrengine
        glfw
    )

    target_include_directories(HelloTriangle PRIVATE
        ${LRENGINE_INCLUDE_DIR}
    )

    # 设置输出目录
    set_target_properties(HelloTriangle PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/examples
    )

    # TexturedCubeGL示例 (OpenGL)
    add_executable(TexturedCubeGL
        TexturedCubeGL/main.cpp
    )

    target_link_libraries(TexturedCubeGL PRIVATE
        lrengine
        glfw
    )

    target_include_directories(TexturedCubeGL PRIVATE
        ${LRENGINE_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
    )

    set_target_properties(TexturedCubeGL PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/examples
    )

    # RenderToTextureGL示例 (OpenGL - 离屏渲染)
    add_executable(RenderToTextureGL
        RenderToTextureGL/main.cpp
    )

    target_link_libraries(RenderToTextureGL PRIVATE
        lrengine
        glfw
    )

    target_include_directories(RenderToTextureGL PRIVATE
        ${LRENGINE_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
    )

    set_target_properties(RenderToTextureGL PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/examples
    )
endif()

# HelloTriangleMTL示例 (Metal)
if(LRENGINE_ENABLE_METAL AND APPLE)
    add_executable(HelloTriangleMTL
        HelloTriangleMTL/main.mm
    )

    target_link_libraries(HelloTriangleMTL PRIVATE
        lrengine
        glfw
        "-framework Metal"
        "-framework QuartzCore"
        "-framework Cocoa"
    )

    target_include_directories(HelloTriangleMTL PRIVATE
        ${LRENGINE_INCLUDE_DIR}
    )

    # 设置Objective-C++编译选项
    set_source_files_properties(HelloTriangleMTL/main.mm PROPERTIES
        COMPILE_FLAGS "-fobjc-arc"
    )

    # 设置输出目录
    set_target_properties(HelloTriangleMTL PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/examples
    )
endif()

# TexturedCubeMTL示例 (Metal)
if(LRENGINE_ENABLE_METAL AND APPLE)
    add_executable(TexturedCubeMTL
        TexturedCubeMTL/main.mm
    )

    target_link_libraries(TexturedCubeMTL PRIVATE
        lrengine
        glfw
        "-framework Metal"
        "-framework QuartzCore"
        "-framework Cocoa"
    )

    target_include_directories(TexturedCubeMTL PRIVATE
        ${LRENGINE_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
    )

    # 设置Objective-C++编译选项
    set_source_files_properties(TexturedCubeMTL/main.mm PROPERTIES
        COMPILE_FLAGS "-fobjc-arc"
    )

    # 设置输出目录
    set_target_properties(TexturedCubeMTL PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/examples
    )
endif()

# RenderToTextureMTL示例 (Metal - 离屏渲染)
if(LRENGINE_ENABLE_METAL AND APPLE)
    add_executable(RenderToTextureMTL
        RenderToTextureMTL/main.mm
    )

    target_link_libraries(RenderToTextureMTL PRIVATE
        lrengine
        glfw
        "-framework Metal"
        "-framework QuartzCore"
        "-framework Cocoa"
    )

    target_include_directories(RenderToTextureMTL PRIVATE
        ${LRENGINE_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
    )

    # 设置Objective-C++编译选项
    set_source_files_properties(RenderToTextureMTL/main.mm PROPERTIES
        COMPILE_FLAGS "-fobjc-arc"
    )

    # 设置输出目录
    set_target_properties(RenderToTextureMTL PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/examples
    )
endif()
