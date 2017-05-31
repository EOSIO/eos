
file (GLOB SOURCE_FILES *.cpp)
file (GLOB HEADER_FILES *.hpp)

init_target (associative_storage)

build_executable (${TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES})

link_boost ()
final_target ()

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "examples")
