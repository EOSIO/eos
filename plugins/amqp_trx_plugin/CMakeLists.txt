file(GLOB HEADERS "include/eosio/amqp_trx_plugin/*.hpp")
add_library( amqp_trx_plugin
             amqp_trx_plugin.cpp
             amqp_trace_plugin_impl.cpp
             ${HEADERS} )

target_link_libraries( amqp_trx_plugin state_history chain_plugin producer_plugin abieos appbase fc amqp amqpcpp )
target_include_directories( amqp_trx_plugin PUBLIC include )

add_subdirectory(test)
