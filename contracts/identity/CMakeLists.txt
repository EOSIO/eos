file(GLOB ABI_FILES "*.abi")
configure_file("${ABI_FILES}" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)
add_wast_executable(TARGET identity
  INCLUDE_FOLDERS "${STANDARD_INCLUDE_FOLDERS}"
  LIBRARIES eosiolib
  DESTINATION_FOLDER ${CMAKE_CURRENT_BINARY_DIR}
)

add_subdirectory(test)
