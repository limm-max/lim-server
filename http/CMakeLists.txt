# ========================================
# http模块：http框架
# ========================================

set(HTTP_SOURCES
    src/HttpRequest.cc
    src/HttpContext.cc
    src/HttpResponse.cc
    src/HttpServer.cc
    src/Router.cc
    src/MiddlewareChain.cc    
    src/CoresMiddleware.cc
    src/Session.cc             
    src/SessionStorage.cc      
    src/SessionManager.cc      
)

add_library(lim_http STATIC ${HTTP_SOURCES})

target_include_directories(lim_http PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(lim_http PUBLIC lim_net lim_base)