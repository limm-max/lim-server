# ========================================
# base 模块：基础设施
# ========================================

# 收集 base/src 下所有 .cc 文件
file(GLOB BASE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")

# 现在 src 目录还是空的，先做一个占位（避免空库报错）
if(BASE_SOURCES)
    add_library(lim_base STATIC ${BASE_SOURCES})
    target_include_directories(lim_base PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(lim_base PUBLIC pthread)
    message(STATUS "  [base] Found ${BASE_SOURCES}")
else()
    message(STATUS "  [base] No sources yet, skipping library build")
endif()