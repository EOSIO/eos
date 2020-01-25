file(GLOB HEADERS "include/eosio/db_size_api_plugin/*.hpp")
add_library( db_size_api_plugin
             db_size_api_plugin.cpp
             ${HEADERS} )

target_link_libraries( db_size_api_plugin http_plugin chain_plugin )
target_include_directories( db_size_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
