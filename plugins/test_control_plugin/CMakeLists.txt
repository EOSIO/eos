file(GLOB HEADERS "include/eosio/test_control_plugin/*.hpp")

add_library( test_control_plugin
             test_control_plugin.cpp
             ${HEADERS} )

target_link_libraries( test_control_plugin producer_plugin chain_plugin http_client_plugin appbase eosio_chain )
target_include_directories( test_control_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
