# file(GLOB ABI_FILES "*.abi")
# configure_file("${ABI_FILES}" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)

# add_wast_executable(TARGET test_ram_limit
#   INCLUDE_FOLDERS ${STANDARD_INCLUDE_FOLDERS}
#   LIBRARIES libc++ libc eosiolib
#   DESTINATION_FOLDER ${CMAKE_CURRENT_BINARY_DIR}
# )

set( CMAKE_CXX_STANDARD 17 )

add_contract( test_ram_limit test_ram_limit test_ram_limit.cpp )
