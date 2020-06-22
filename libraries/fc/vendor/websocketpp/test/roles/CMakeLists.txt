# Test client role
file (GLOB SOURCE client.cpp)

init_target (test_roles_client)
build_test (${TARGET_NAME} ${SOURCE})
link_boost ()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")

# Test server role
file (GLOB SOURCE server.cpp)

init_target (test_roles_server)
build_test (${TARGET_NAME} ${SOURCE})
link_boost ()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")
