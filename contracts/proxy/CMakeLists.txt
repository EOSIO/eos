set( CMAKE_CXX_STANDARD 17 )

if( ${eosio.cdt_FOUND} )
  include_directories( ${MYPROJECT_DIR}../ )
  add_contract_no_abi( proxy proxy proxy.cpp )
else()
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/proxy.wasm ${CMAKE_CURRENT_BINARY_DIR}/proxy.wasm COPYONLY )
endif()

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/proxy.abi ${CMAKE_CURRENT_BINARY_DIR}/proxy.abi COPYONLY )
