file(GLOB HEADERS "include/eosio/zmq_plugin/*.hpp")
add_library( zmq_plugin
             zmq_plugin.cpp
             ${HEADERS} )

target_link_libraries( zmq_plugin chain_plugin eosio_chain appbase fc )
target_include_directories( zmq_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
