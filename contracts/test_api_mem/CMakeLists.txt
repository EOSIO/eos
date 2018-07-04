#set the MAX_MEMORY to 1MB for these tests; there were lots of memory unit tests that assume such

add_wast_executable( TARGET test_api_mem
   INCLUDE_FOLDERS "${STANDARD_INCLUDE_FOLDERS}"
   LIBRARIES eosiolib
   DESTINATION_FOLDER ${CMAKE_CURRENT_BINARY_DIR}
   MAX_MEMORY 1048576
)
