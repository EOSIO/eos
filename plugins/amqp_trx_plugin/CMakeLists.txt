file(GLOB HEADERS "include/eosio/amqp_trx_plugin/*.hpp")
add_library( amqp_trx_plugin
             amqp_trx_plugin.cpp
             ${HEADERS} )

target_link_libraries( amqp_trx_plugin amqp_trace_plugin chain_plugin producer_plugin appbase fc amqpcpp )
target_include_directories( amqp_trx_plugin PUBLIC include )

add_subdirectory(test)
