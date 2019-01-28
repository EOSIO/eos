if( ${eosio.cdt_FOUND} )
  add_executable( asserter asserter asserter.cpp )
  target_include_directories(asserter PUBLIC ${CMAKE_CURRENT_SOURCE}/../)
else()
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/asserter.wasm ${CMAKE_CURRENT_BINARY_DIR}/asserter.wasm COPYONLY )
endif()

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/asserter.abi ${CMAKE_CURRENT_BINARY_DIR}/asserter.abi COPYONLY )
