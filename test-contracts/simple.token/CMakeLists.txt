set( CMAKE_CXX_STANDARD 17 )

if( ${eosio.cdt_FOUND} )
  add_contract( simple.token simple.token simple.token.cpp )
else()
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/simple.token.wasm ${CMAKE_CURRENT_BINARY_DIR}/simple.token.wasm COPYONLY )
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/simple.token.abi ${CMAKE_CURRENT_BINARY_DIR}/simple.token.abi COPYONLY )
endif()
