file(GLOB HEADERS "include/eosio/witness_trx_plugin/*.hpp")
add_library( witness_trx_plugin
             witness_trx_plugin.cpp
             ${HEADERS} )

target_link_libraries( witness_trx_plugin witness_plugin )
target_include_directories( witness_trx_plugin PUBLIC include )
