add_executable( test_message_buffer test_message_buffer.cpp)
target_link_libraries( test_message_buffer fc )

add_test(NAME test_message_buffer COMMAND libraries/fc/test/network/test_message_buffer WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
