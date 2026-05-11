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