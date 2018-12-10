set( CMAKE_CXX_STANDARD 17 )

if( ${eosio.cdt_FOUND} )
  add_contract_no_abi( test_api_mem test_api_mem test_api_mem.cpp )
else()
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/test_api_mem.wasm ${CMAKE_CURRENT_BINARY_DIR}/test_api_mem.wasm COPYONLY )  
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/test_api_mem.abi ${CMAKE_CURRENT_BINARY_DIR}/test_api_mem.abi COPYONLY )
endif()
