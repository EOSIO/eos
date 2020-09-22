file(GLOB HEADERS "include/eosio/web_client_plugin/*.hpp")
add_library( web_client_plugin
             web_client_plugin.cpp
             beast_client.cpp
             ${HEADERS} )

target_link_libraries( web_client_plugin appbase fc web_server_plugin)
target_include_directories( web_client_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
