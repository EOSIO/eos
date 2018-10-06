add_wast_executable(TARGET test_api_multi_index
  INCLUDE_FOLDERS "${STANDARD_INCLUDE_FOLDERS}"
  LIBRARIES libc++ libc arisenlib
  DESTINATION_FOLDER ${CMAKE_CURRENT_BINARY_DIR}
)
