add_wast_executable(TARGET test_api_multi_index
  INCLUDE_FOLDERS "${STANDARD_INCLUDE_FOLDERS}"
  LIBRARIES libc++ libc enulib
  DESTINATION_FOLDER ${CMAKE_CURRENT_BINARY_DIR}
)
