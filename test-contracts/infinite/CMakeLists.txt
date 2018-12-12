set( CMAKE_CXX_STANDARD 17 )

if( ${eosio.cdt_FOUND} )
  add_contract( infinite infinite infinite.cpp )
else()
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/infinite.wasm ${CMAKE_CURRENT_BINARY_DIR}/infinite.wasm COPYONLY )
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/infinite.abi ${CMAKE_CURRENT_BINARY_DIR}/infinite.abi COPYONLY )
endif()
