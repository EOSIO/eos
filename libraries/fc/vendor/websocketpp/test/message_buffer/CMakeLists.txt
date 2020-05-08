# Test alloc message buffer strategy
file (GLOB SOURCE alloc.cpp)

init_target (test_message_alloc)
build_executable (${TARGET_NAME} ${SOURCE})
link_boost ()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")

# Test message buffers
file (GLOB SOURCE message.cpp)

init_target (test_message_buffer)
build_test (${TARGET_NAME} ${SOURCE})
link_boost ()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")
