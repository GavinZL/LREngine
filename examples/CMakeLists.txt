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
