set( CMAKE_CXX_STANDARD 17 )

include_directories( ${MYPROJECT_DIR}../ )

add_contract_no_abi( proxy proxy proxy.cpp )

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/proxy.abi ${CMAKE_CURRENT_BINARY_DIR}/proxy.abi COPYONLY )
