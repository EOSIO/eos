add_executable( test_router test_router.cpp )
target_link_libraries( test_router net_plugin )

add_test(NAME test_router COMMAND plugins/trace_api_plugin/test/test_router WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

file(GLOB HEADERS "include/eosio/net_plugin/*.hpp" )
add_library( test_generic_message_plugin
             test_generic_message_plugin.cpp
             ${HEADERS} )

target_link_libraries( test_generic_message_plugin net_plugin chain_plugin producer_plugin appbase fc )
target_include_directories( test_generic_message_plugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include )

file(GLOB HEADERS "include/eosio/net_plugin/*.hpp" )
add_library( test_generic_message_api_plugin
        test_generic_message_api_plugin.cpp
        ${HEADERS} )

target_link_libraries( test_generic_message_api_plugin http_plugin test_generic_message_plugin net_plugin chain_plugin producer_plugin appbase fc )
target_include_directories( test_generic_message_plugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include )