# Generic processor tests
file (GLOB SOURCE processor.cpp)

init_target (test_processor)
build_test (${TARGET_NAME} ${SOURCE})
link_boost ()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")

# Hybi00 processor tests
file (GLOB SOURCE hybi00.cpp)

init_target (test_processor_hybi00)
build_test (${TARGET_NAME} ${SOURCE})
link_boost ()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")

# Hybi07 processor tests
file (GLOB SOURCE hybi07.cpp)

init_target (test_processor_hybi07)
build_test (${TARGET_NAME} ${SOURCE})
link_boost ()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")

# Hybi08 processor tests
file (GLOB SOURCE hybi08.cpp)

init_target (test_processor_hybi08)
build_test (${TARGET_NAME} ${SOURCE})
link_boost ()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")

if (ZLIB_FOUND)

# Hybi13 processor tests
file (GLOB SOURCE hybi13.cpp)

init_target (test_processor_hybi13)
build_test (${TARGET_NAME} ${SOURCE})
link_boost ()
link_zlib()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")

# Permessage compression extension processor tests
file (GLOB SOURCE extension_permessage_compress.cpp)

init_target (test_processor_extension_permessage_compress)
build_test (${TARGET_NAME} ${SOURCE})
link_boost ()
link_zlib()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")

endif ( ZLIB_FOUND )