add_executable( test_account_query_db test_account_query_db.cpp )

target_link_libraries( test_account_query_db chain_plugin eosio_testing)

add_test(NAME test_account_query_db COMMAND plugins/chain_plugin/test/test_account_query_db WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
