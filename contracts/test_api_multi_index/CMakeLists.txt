set( CMAKE_CXX_STANDARD 17 )

if( ${eosio.cdt_FOUND} )
  include_directories( ${Boost_INCLUDE_DIRS} )
  add_contract( test_api_multi_index test_api_multi_index test_api_multi_index.cpp )
else()
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/test_api_multi_index.wasm ${CMAKE_CURRENT_BINARY_DIR}/test_api_multi_index.wasm COPYONLY )
endif()
