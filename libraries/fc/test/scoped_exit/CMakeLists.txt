add_executable( test_scoped_exit test_scoped_exit.cpp )
target_link_libraries( test_scoped_exit fc )

add_test(NAME test_scope_exit COMMAND libraries/fc/test/scoped_exit/test_scoped_exit WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
