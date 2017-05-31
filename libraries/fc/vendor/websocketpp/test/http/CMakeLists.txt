
file (GLOB SOURCE_FILES parser.cpp)

init_target (test_http)

build_test (${TARGET_NAME} ${SOURCE_FILES})

link_boost ()
final_target ()

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")
