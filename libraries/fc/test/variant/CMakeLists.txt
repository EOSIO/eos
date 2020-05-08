add_executable( test_variant test_variant.cpp )
target_link_libraries( test_variant fc )

add_test(NAME test_variant COMMAND libraries/fc/test/variant/test_variant WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
