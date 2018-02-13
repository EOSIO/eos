
file(GLOB ABI_FILES "*.abi")
set(ABI_FILES "multi_index_test.abi")
add_wast_executable(TARGET multi_index_test
  INCLUDE_FOLDERS "${STANDARD_INCLUDE_FOLDERS}" /usr/local/include
  LIBRARIES libc++ libc eosiolib
  DESTINATION_FOLDER ${CMAKE_CURRENT_BINARY_DIR}
)

configure_file("${ABI_FILES}" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)
