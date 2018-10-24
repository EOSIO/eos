file(GLOB HEADERS "include/eosio/chain_to_mongo_plugin/*.hpp")
add_library( chain_to_mongo_plugin
             chain_to_mongo_plugin.cpp
             ${HEADERS} )

target_link_libraries( chain_to_mongo_plugin http_plugin chain_plugin )
target_include_directories( chain_to_mongo_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
