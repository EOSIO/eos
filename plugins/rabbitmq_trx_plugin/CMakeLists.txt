file(GLOB HEADERS "include/eosio/rabbitmq_trx_plugin/*.hpp")
add_library( rabbitmq_trx_plugin
             rabbitmq_trx_plugin.cpp
             ${HEADERS} )

target_link_libraries( rabbitmq_trx_plugin chain_plugin appbase fc amqpcpp )
target_include_directories( rabbitmq_trx_plugin PUBLIC include )
