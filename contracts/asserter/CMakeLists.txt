set( CMAKE_CXX_STANDARD 17 )

if( ${eosio.cdt_FOUND} )
  include_directories( ${MYPROJECT_DIR}../ )
  add_contract_no_abi( asserter asserter asserter.cpp )

else()
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/asserter.wasm ${CMAKE_CURRENT_BINARY_DIR}/asserter.wasm COPYONLY )
endif()

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/asserter.abi ${CMAKE_CURRENT_BINARY_DIR}/asserter.abi COPYONLY )
