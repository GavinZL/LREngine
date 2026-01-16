# Examples CMakeLists.txt

# HelloTriangle示例
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
