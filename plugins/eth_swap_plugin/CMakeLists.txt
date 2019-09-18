file(GLOB HEADERS "include/eosio/eth_swap_plugin/*.hpp")
add_library( eth_swap_plugin
             my_web3.cpp
             eth_swap_plugin.cpp
             ${HEADERS} )

target_link_libraries( eth_swap_plugin appbase fc http_plugin chain_plugin eosio_testing )
target_include_directories( eth_swap_plugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
                            eth_swap_plugin PUBLIC ${CMAKE_SOURCE_DIR}/libraries/testing/include
                            eth_swap_plugin PUBLIC ${CMAKE_BINARY_DIR}/unittests/include )
