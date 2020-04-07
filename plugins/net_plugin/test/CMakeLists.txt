add_executable( test_router test_router.cpp )
target_link_libraries( test_router net_plugin )

add_test(NAME test_router COMMAND plugins/trace_api_plugin/test/test_router WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
