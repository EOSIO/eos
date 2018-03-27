file(GLOB HEADERS "include/eosio/txn_test_gen_plugin/*.hpp")
add_library( txn_test_gen_plugin
             txn_test_gen_plugin.cpp
             ${HEADERS} )

target_link_libraries( txn_test_gen_plugin appbase fc http_plugin chain_plugin )
target_include_directories( txn_test_gen_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
target_include_directories( txn_test_gen_plugin PUBLIC ${CMAKE_BINARY_DIR}/contracts )

add_dependencies( txn_test_gen_plugin currency )
