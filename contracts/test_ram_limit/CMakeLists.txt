set( CMAKE_CXX_STANDARD 17 )

if( ${eosio.cdt_FOUND} )
  add_contract( test_ram_limit test_ram_limit test_ram_limit.cpp )
else()
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/test_ram_limit.wasm ${CMAKE_CURRENT_BINARY_DIR}/test_ram_limit.wasm COPYONLY )  
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/test_ram_limit.abi ${CMAKE_CURRENT_BINARY_DIR}/test_ram_limit.abi COPYONLY )
endif()
