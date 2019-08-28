file(GLOB HEADERS "include/eosio/topology_api_plugin/*.hpp")
add_library( topology_api_plugin
             topology_api_plugin.cpp
             ${HEADERS} )

target_link_libraries( topology_api_plugin topology_plugin http_plugin appbase )
target_include_directories( topology_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
