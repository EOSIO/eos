file(GLOB SOURCE_FILES "*.cpp")
configure_file("${ABI_FILES}" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)
add_wast_target(table_abi_test "${CMAKE_SOURCE_DIR}/contracts" ${CMAKE_CURRENT_BINARY_DIR})
