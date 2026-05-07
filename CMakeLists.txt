# ========================================
# lim-server: 顶层构建文件
# ========================================
cmake_minimum_required(VERSION 3.10)
project(lim_server
    VERSION 0.1.0
    DESCRIPTION "A high-performance C++ web server framework"
    LANGUAGES CXX
)

# ----------- C++ 标准 -----------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ----------- 编译选项 -----------
# 默认 Debug 模式（开发时方便调试）
if(NOT CMAKE_BUILD_TYPE)    #CMAKE_BUILD_TYPE,构造类型
    set(CMAKE_BUILD_TYPE Debug)
endif()

# 编译选项
add_compile_options(
    -Wall              # 开启所有警告
    -Wextra            # 额外警告
    -Wno-unused-parameter  # 但允许未使用参数（muduo 风格里很多）
    -g                 # 调试信息
)

# ----------- 头文件搜索路径 -----------
#PROJECT_SOURCE_DIR：CMake内置变量，项目根目录的绝对路径
include_directories(
    ${PROJECT_SOURCE_DIR}/base/include
    ${PROJECT_SOURCE_DIR}/net/include
    ${PROJECT_SOURCE_DIR}/http/include
)

# ----------- 输出路径 -----------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)   #可执行文件
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)   #动态库
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)   #静态库

# ----------- 添加子模块 -----------
add_subdirectory(base)
# add_subdirectory(net)       # 后面再开
# add_subdirectory(http)      # 后面再开
# add_subdirectory(examples)  # 后面再开
add_subdirectory(tests)     # 后面再开

# ----------- 打印一下信息 -----------
message(STATUS "==========================================")
message(STATUS "lim-server build configuration:")
message(STATUS "  Build Type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler:     ${CMAKE_CXX_COMPILER}")
message(STATUS "==========================================")