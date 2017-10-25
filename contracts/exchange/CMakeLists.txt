file(GLOB ABI_FILES "*.abi")
add_wast_target(exchange "${CMAKE_SOURCE_DIR}/contracts" ${CMAKE_CURRENT_BINARY_DIR})
configure_file("${ABI_FILES}" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)
add_dependencies( exchange currency )
