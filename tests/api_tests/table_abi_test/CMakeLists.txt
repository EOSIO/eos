file(GLOB SOURCE_FILES "*.cpp")
add_wast_abi_target(table_abi_test "${SOURCE_FILES}" "${CMAKE_SOURCE_DIR}/contracts" ${CMAKE_CURRENT_BINARY_DIR})
