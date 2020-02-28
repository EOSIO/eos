add_executable( test_extraction test_extraction.cpp )
target_link_libraries( test_extraction trace_api_plugin )
target_include_directories( test_extraction PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_test(NAME test_extraction COMMAND plugins/trace_api_plugin/test/test_extraction WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable( test_logging test_logging.cpp )
target_link_libraries( test_logging trace_api_plugin )
target_include_directories( test_logging PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_test(NAME test_logging COMMAND plugins/trace_api_plugin/test/test_logging WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable( test_responses test_responses.cpp )
target_link_libraries( test_responses trace_api_plugin )
target_include_directories( test_responses PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_test(NAME test_responses COMMAND plugins/trace_api_plugin/test/test_responses WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
