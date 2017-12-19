file(GLOB SOURCE_FILES "*.cpp")
add_wast_abi_target(table_abi_test "${CMAKE_SOURCE_DIR}/contracts" ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
