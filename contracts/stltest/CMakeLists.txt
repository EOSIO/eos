set( CMAKE_CXX_STANDARD 17 )

if( ${eosio.cdt_FOUND} )
  add_contract( stltest stltest stltest.cpp )
else()
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/stltest.wasm ${CMAKE_CURRENT_BINARY_DIR}/stltest.wasm COPYONLY )
endif()

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/stltest.abi ${CMAKE_CURRENT_BINARY_DIR}/stltest.abi COPYONLY )
