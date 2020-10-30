add_executable( test_ordered test_ordered.cpp )
target_link_libraries( test_ordered amqp_trx_plugin eosio_testing )
target_include_directories( test_ordered PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_test(NAME test_ordered COMMAND plugins/amqp_trx_plugin/test/test_ordered WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable( test_ordered_full test_ordered_full.cpp )
target_link_libraries( test_ordered_full amqp_trx_plugin eosio_testing )
target_include_directories( test_ordered_full PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_test(NAME test_ordered_full COMMAND plugins/amqp_trx_plugin/test/test_ordered_full WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
