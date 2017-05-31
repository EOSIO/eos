# Test close utilities
file (GLOB SOURCE close.cpp)

init_target (test_close)
build_test (${TARGET_NAME} ${SOURCE})
link_boost ()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")

# Test error utilities
file (GLOB SOURCE error.cpp)

init_target (test_error)
build_test (${TARGET_NAME} ${SOURCE})
link_boost ()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")

# Test frame utilities
file (GLOB SOURCE frame.cpp)

init_target (test_frame)
build_test (${TARGET_NAME} ${SOURCE})
link_boost ()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")

# Test sha1 utilities
file (GLOB SOURCE sha1.cpp)

init_target (test_sha1)
build_test (${TARGET_NAME} ${SOURCE})
link_boost ()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")

# Test uri utilities
file (GLOB SOURCE uri.cpp)

init_target (test_uri)
build_test (${TARGET_NAME} ${SOURCE})
link_boost ()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")

# Test misc utilities
file (GLOB SOURCE utilities.cpp)

init_target (test_utilities)
build_test (${TARGET_NAME} ${SOURCE})
link_boost ()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")
