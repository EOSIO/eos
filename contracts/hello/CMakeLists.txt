set( CMAKE_CXX_STANDARD 17 )

if( ${eosio.cdt_FOUND} )
  add_contract( hello hello hello.cpp )
else()
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/hello.wasm ${CMAKE_CURRENT_BINARY_DIR}/hello.wasm COPYONLY )  
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/hello.abi ${CMAKE_CURRENT_BINARY_DIR}/hello.abi COPYONLY )
endif()
