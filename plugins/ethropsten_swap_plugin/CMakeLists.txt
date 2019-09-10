file(GLOB HEADERS "include/eosio/ethropsten_swap_plugin/*.hpp")
add_library( ethropsten_swap_plugin
             my_web3.cpp
             ethropsten_swap_plugin.cpp
             ${HEADERS} )

target_link_libraries( ethropsten_swap_plugin appbase fc http_plugin chain_plugin eosio_testing )
target_include_directories( ethropsten_swap_plugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
                            ethropsten_swap_plugin PUBLIC ${CMAKE_SOURCE_DIR}/libraries/testing/include
                            ethropsten_swap_plugin PUBLIC ${CMAKE_BINARY_DIR}/unittests/include )
