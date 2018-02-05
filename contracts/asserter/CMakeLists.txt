file(GLOB ABI_FILES "*.abi")
configure_file("${ABI_FILES}" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)
add_wast_target(asserter "${CMAKE_SOURCE_DIR}/contracts" ${CMAKE_CURRENT_BINARY_DIR})
