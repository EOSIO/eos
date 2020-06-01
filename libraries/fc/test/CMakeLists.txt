add_subdirectory( crypto )
add_subdirectory( io )
add_subdirectory( network )
add_subdirectory( scoped_exit )
add_subdirectory( static_variant )
add_subdirectory( variant )

add_executable( test_base64 test_base64.cpp )
target_link_libraries( test_base64 fc )

add_test(NAME test_base64 COMMAND libraries/fc/test/test_base64 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable( test_time test_time.cpp )
target_link_libraries( test_time fc )

add_test(NAME test_time COMMAND libraries/fc/test/test_time WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
