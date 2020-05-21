file(GLOB HEADERS "include/eosio/amqp_witness_plugin/*.hpp")
add_library( amqp_witness_plugin
             amqp_witness_plugin.cpp
             ${HEADERS} )

target_link_libraries( amqp_witness_plugin appbase fc signature_provider_plugin chain_plugin amqpcpp reliable_amqp_publisher)
target_include_directories( amqp_witness_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
