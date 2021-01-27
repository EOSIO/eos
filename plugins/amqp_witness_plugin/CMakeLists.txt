file(GLOB HEADERS "include/eosio/amqp_witness_plugin/*.hpp")
add_library( amqp_witness_plugin
             amqp_witness_plugin.cpp
             ${HEADERS} )

target_link_libraries( amqp_witness_plugin appbase fc witness_plugin amqp)
target_include_directories( amqp_witness_plugin PUBLIC include )
