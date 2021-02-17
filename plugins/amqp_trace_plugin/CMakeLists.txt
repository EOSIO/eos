file(GLOB HEADERS "include/eosio/amqp_trace_plugin/*.hpp")
add_library( amqp_trace_plugin
             amqp_trace_plugin.cpp
             amqp_trace_plugin_impl.cpp
             ${HEADERS} )

target_link_libraries( amqp_trace_plugin state_history chain_plugin amqp abieos appbase fc amqpcpp )
target_include_directories( amqp_trace_plugin PUBLIC include )
