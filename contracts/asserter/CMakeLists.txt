set( CMAKE_CXX_STANDARD 17 )

include_directories( ${MYPROJECT_DIR}../ )

add_contract_no_abi( asserter asserter asserter.cpp )

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/asserter.abi ${CMAKE_CURRENT_BINARY_DIR}/asserter.abi COPYONLY )
