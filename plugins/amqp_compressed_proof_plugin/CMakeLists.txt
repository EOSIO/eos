file(GLOB HEADERS "include/eosio/amqp_compressed_proof_plugin/*.hpp")
add_library( amqp_compressed_proof_plugin
             amqp_compressed_proof_plugin.cpp
             ${HEADERS} )

target_link_libraries( amqp_compressed_proof_plugin appbase fc chain_plugin reliable_amqp_publisher )
target_include_directories( amqp_compressed_proof_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
