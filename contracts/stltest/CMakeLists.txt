set( CMAKE_CXX_STANDARD 17 )

add_contract( stltest stltest stltest.cpp )

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/stltest.abi ${CMAKE_CURRENT_BINARY_DIR}/stltest.abi COPYONLY )
