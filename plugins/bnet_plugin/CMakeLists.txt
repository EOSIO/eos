file(GLOB HEADERS "include/eosio/bnet_plugin/*.hpp")
add_library( bnet_plugin
             bnet_plugin.cpp
             ${HEADERS} )

target_link_libraries( bnet_plugin chain_plugin eosio_chain appbase )
target_include_directories( bnet_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
