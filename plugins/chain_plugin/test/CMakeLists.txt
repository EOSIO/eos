add_executable( test_account_query_db test_account_query_db.cpp )

target_link_libraries( test_account_query_db chain_plugin eosio_testing)

target_include_directories( test_account_query_db PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../../../libraries/chain/include" "${CMAKE_CURRENT_SOURCE_DIR}/../../../libraries/chainbase/include")

add_test(NAME test_account_query_db COMMAND plugins/chain_plugin/test/test_account_query_db WORKING_DIRECTORY ${CMAKE_BINARY_DIR})