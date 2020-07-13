file(GLOB HEADERS "include/eosio/amqp_trace_plugin/*.hpp")
add_library( amqp_trace_plugin
             amqp_trace_plugin.cpp
             ${HEADERS} )

target_link_libraries( amqp_trace_plugin chain_plugin appbase fc amqpcpp )
target_include_directories( amqp_trace_plugin PUBLIC include )
