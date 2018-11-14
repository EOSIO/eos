# file(GLOB ABI_FILES "*.abi")
# configure_file("${ABI_FILES}" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)

# add_wast_executable(TARGET deferred_test
#   INCLUDE_FOLDERS "${STANDARD_INCLUDE_FOLDERS}" ${Boost_INCLUDE_DIR}
#   LIBRARIES libc++ libc eosiolib
#   DESTINATION_FOLDER ${CMAKE_CURRENT_BINARY_DIR}
# )

set( CMAKE_CXX_STANDARD 17 )

# include_directories( ${MYPROJECT_DIR}../ )

# add_contract_no_abi( asserter asserter asserter.cpp )
add_contract( deferred_test deferred_test deferred_test.cpp )

# configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/asserter.abi ${CMAKE_CURRENT_BINARY_DIR}/asserter.abi COPYONLY )
