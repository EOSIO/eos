add_executable( test_threshold test_threshold.cpp )
target_link_libraries( test_threshold resource_monitor_plugin )
target_include_directories( test_threshold PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_test(NAME test_threshold COMMAND plugins/resource_monitor_plugin/test/test_threshold WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable( test_monitor_loop test_monitor_loop.cpp )
target_link_libraries( test_monitor_loop resource_monitor_plugin )
target_include_directories( test_monitor_loop PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_test(NAME test_monitor_loop COMMAND plugins/resource_monitor_plugin/test/test_monitor_loop WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable( test_add_file_system test_add_file_system.cpp )
target_link_libraries( test_add_file_system resource_monitor_plugin )
target_include_directories( test_add_file_system PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_test(NAME test_add_file_system COMMAND plugins/resource_monitor_plugin/test/test_add_file_system WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable( test_resmom_impl test_resmom_impl.cpp )
target_link_libraries( test_resmom_impl resource_monitor_plugin )
target_include_directories( test_resmom_impl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
add_test(NAME test_resmom_impl COMMAND plugins/resource_monitor_plugin/test/test_resmom_impl WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
