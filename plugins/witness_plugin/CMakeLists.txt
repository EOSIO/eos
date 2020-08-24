file(GLOB HEADERS "include/eosio/witness_plugin/*.hpp")
add_library( witness_plugin
             witness_plugin.cpp
             ${HEADERS} )

target_link_libraries( witness_plugin appbase fc signature_provider_plugin chain_plugin )
target_include_directories( witness_plugin PUBLIC include )
