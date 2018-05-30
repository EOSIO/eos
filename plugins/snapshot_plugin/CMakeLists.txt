file(GLOB HEADERS "include/eosio/snapshot_plugin/*.hpp")
add_library( snapshot_plugin
             snapshot_plugin.cpp
             ${HEADERS} )

target_link_libraries( snapshot_plugin appbase fc chain_plugin)
target_include_directories( snapshot_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
