# Extension Tests
file (GLOB SOURCE extension.cpp)

init_target (test_extension)
build_executable (${TARGET_NAME} ${SOURCE})
link_boost ()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")

if ( ZLIB_FOUND )

# Permessage-deflate tests
file (GLOB SOURCE permessage_deflate.cpp)

init_target (test_permessage_deflate)
build_test (${TARGET_NAME} ${SOURCE})
link_boost ()
link_zlib()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")

endif ( ZLIB_FOUND )