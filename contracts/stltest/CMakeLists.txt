#file(GLOB ABI_FILES "*.abi")
set(ABI_FILES "stltest.abi")
add_wast_target(stltest "${STANDARD_INCLUDE_FOLDERS}" ${CMAKE_CURRENT_BINARY_DIR})
configure_file("${ABI_FILES}" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)
