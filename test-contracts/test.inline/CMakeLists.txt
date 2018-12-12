set( CMAKE_CXX_STANDARD 17 )

if( ${eosio.cdt_FOUND} )
  include_directories( ${MYPROJECT_DIR}../ )
  add_contract( test.inline test.inline test.inline.cpp )
else()
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/test.inline.wasm ${CMAKE_CURRENT_BINARY_DIR}/test.inline.wasm COPYONLY )
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/test.inline.abi ${CMAKE_CURRENT_BINARY_DIR}/test.inline.abi COPYONLY )
endif()
