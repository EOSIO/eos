set( CMAKE_CXX_STANDARD 17 )

if( ${eosio.cdt_FOUND} )
  include_directories( ${MYPROJECT_DIR}../ )
  add_contract( test_api test_api test_api.cpp )
else()
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/test_api.wasm ${CMAKE_CURRENT_BINARY_DIR}/test_api.wasm COPYONLY )
endif()
