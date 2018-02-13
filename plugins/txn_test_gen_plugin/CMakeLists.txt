if(WASM_TOOLCHAIN)
file(GLOB HEADERS "include/eosio/txn_test_gen_plugin/*.hpp")
add_library( txn_test_gen_plugin
             txn_test_gen_plugin.cpp
             ${HEADERS} )

target_link_libraries( txn_test_gen_plugin appbase fc http_plugin chain_plugin )
target_include_directories( txn_test_gen_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
target_include_directories( txn_test_gen_plugin PUBLIC ${CMAKE_BINARY_DIR}/contracts )

add_dependencies( txn_test_gen_plugin currency)

install( TARGETS
   txn_test_gen_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/eosio/txn_test_gen_plugin" )

endif()
