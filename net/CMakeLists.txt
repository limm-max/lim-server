# ========================================
# net模块：网络库模块
# ========================================

set(NET_SOURCES
    src/Channel.cc
    src/EventLoop.cc
    src/EventLoopThread.cc  
    src/EventLoopThreadPool.cc
    src/InetAddress.cc
    src/Socket.cc
    src/SocketsOps.cc       
    src/Acceptor.cc  
    src/Buffer.cc 
    src/TcpConnection.cc    
    src/TcpServer.cc  
    poller/src/Poller.cc
    poller/src/DefaultPoller.cc
    poller/src/EPollPoller.cc


    
    )

add_library(lim_net STATIC ${NET_SOURCES})

target_include_directories(lim_net PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/poller/include
)

# Thread 用到了 pthread,必须链接
target_link_libraries(lim_net PUBLIC lim_base)

target_compile_features(lim_net PUBLIC cxx_std_17)