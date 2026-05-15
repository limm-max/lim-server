add_executable(test_timestamp test_timestamp.cc)
target_link_libraries(test_timestamp lim_base)

add_executable(test_thread test_thread.cc)
target_link_libraries(test_thread lim_base)

add_executable(test_sync test_sync.cc)
target_link_libraries(test_sync lim_base)

add_executable(test_logstream test_logstream.cc)
target_link_libraries(test_logstream lim_base)

add_executable(test_asynclogging test_asynclogging.cc)
target_link_libraries(test_asynclogging lim_base)

add_executable(test_eventloop test_eventloop.cc)
target_link_libraries(test_eventloop PRIVATE lim_net pthread)

add_executable(test_eventloopthread test_eventloopthread.cc)
target_link_libraries(test_eventloopthread PRIVATE lim_net pthread)

add_executable(test_eventloopthreadpool test_eventloopthreadpool.cc)
target_link_libraries(test_eventloopthreadpool PRIVATE lim_net pthread)

add_executable(test_socket test_socket.cc)
target_link_libraries(test_socket PRIVATE lim_net pthread)

add_executable(test_acceptor test_acceptor.cc)
target_link_libraries(test_acceptor PRIVATE lim_net pthread)
 
add_executable(test_buffer test_buffer.cc)
target_link_libraries(test_buffer PRIVATE lim_net pthread)