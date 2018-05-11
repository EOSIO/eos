file( GLOB HEADERS "include/eosio/history_api_plugin/*.hpp" )
add_library( history_api_plugin
             history_api_plugin.cpp
             ${HEADERS} )

target_link_libraries( history_api_plugin history_plugin chain_plugin http_plugin appbase )
target_include_directories( history_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
