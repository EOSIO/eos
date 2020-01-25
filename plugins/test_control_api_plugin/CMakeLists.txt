file(GLOB HEADERS "include/eosio/test_control_api_plugin/*.hpp")
add_library( test_control_api_plugin
             test_control_api_plugin.cpp
             ${HEADERS} )

target_link_libraries( test_control_api_plugin test_control_plugin chain_plugin http_plugin appbase )
target_include_directories( test_control_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
