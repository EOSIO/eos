file(GLOB HEADERS "include/eosio/chain_provider_plugin/*.hpp")
add_library( chain_provider_plugin
             chain_provider_plugin.cpp
             ${HEADERS} )

target_link_libraries( chain_provider_plugin chain_plugin appbase fc )
target_include_directories( chain_provider_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
