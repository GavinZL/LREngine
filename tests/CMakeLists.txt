cmake_minimum_required(VERSION 3.15)

# 测试源文件
set(TEST_SOURCES
    TestLRLog.cpp
)

# 创建测试可执行文件
add_executable(lrengine_tests ${TEST_SOURCES})

# 链接 lrengine 库
target_link_libraries(lrengine_tests PRIVATE lrengine)

# 包含目录
target_include_directories(lrengine_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# 编译定义（确保测试在 Debug 模式下有完整的日志级别）
target_compile_definitions(lrengine_tests PRIVATE LR_DEBUG)

# 注册测试
add_test(NAME LRLogTests COMMAND lrengine_tests)

# 设置工作目录
set_tests_properties(LRLogTests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
