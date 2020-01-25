file(GLOB HEADERS "include/eosio/login_plugin/*.hpp")
add_library( login_plugin
             login_plugin.cpp
             ${HEADERS} )

target_link_libraries( login_plugin chain_plugin http_plugin appbase )
target_include_directories( login_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
