# CMakeLists.txt for LREngine Android Demo
cmake_minimum_required(VERSION 3.22.1)
project(lrenginedemo)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# LREngine库路径
set(LRENGINE_LIB_DIR ${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI})
set(LRENGINE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

# 添加LREngine预编译共享库
add_library(lrengine SHARED IMPORTED)
set_target_properties(lrengine PROPERTIES
    IMPORTED_LOCATION ${LRENGINE_LIB_DIR}/liblrengine.so
)

# 创建本地库
add_library(native-lib SHARED
    native-lib.cpp
)

# 设置头文件路径
target_include_directories(native-lib PRIVATE
    ${LRENGINE_INCLUDE_DIR}
)

# 查找系统库
find_library(log-lib log)
find_library(android-lib android)
find_library(gles3-lib GLESv3)
find_library(egl-lib EGL)

# 链接库
target_link_libraries(native-lib
    lrengine
    ${log-lib}
    ${android-lib}
    ${gles3-lib}
    ${egl-lib}
)
