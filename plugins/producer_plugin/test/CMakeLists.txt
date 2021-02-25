add_executable( test_subjective_billing test_subjective_billing.cpp )
target_link_libraries( test_subjective_billing producer_plugin eosio_testing )

add_test(NAME test_subjective_billing COMMAND plugins/producer_plugin/test/test_subjective_billing WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

