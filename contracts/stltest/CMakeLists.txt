#file(GLOB ABI_FILES "*.abi")
set(ABI_FILES "stltest.abi")
add_wast_executable(TARGET stltest
  INCLUDE_FOLDERS "${STANDARD_INCLUDE_FOLDERS}"
  LIBRARIES eoslib libc
  EXTERNAL_LIBRARIES /Users/antonperkov/final/src/stl.bc
  DESTINATION_FOLDER ${CMAKE_CURRENT_BINARY_DIR}
)

configure_file("${ABI_FILES}" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)
