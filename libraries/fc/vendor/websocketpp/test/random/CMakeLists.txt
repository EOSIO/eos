# Test RNG policy none
file (GLOB SOURCE none.cpp)

init_target (test_random_none)
build_test (${TARGET_NAME} ${SOURCE})
link_boost ()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")

# Test RNG policy random_device
file (GLOB SOURCE random_device.cpp)

init_target (test_random_random_device)
build_test (${TARGET_NAME} ${SOURCE})
link_boost ()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")
