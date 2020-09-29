file(GLOB HEADERS "include/eosio/web_server_plugin/*.hpp")
add_library( web_server_plugin
             web_server_plugin.cpp
             beast_server.cpp
             ${HEADERS} )

target_link_libraries( web_server_plugin chain appbase fc )
target_include_directories( web_server_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
