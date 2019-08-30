file(GLOB HEADERS "include/eosio/swap_plugin/*.hpp")
add_library( swap_plugin
             swap_plugin.cpp
             ${HEADERS} )

target_link_libraries( swap_plugin appbase fc http_plugin chain_plugin eosio_testing )
target_include_directories( swap_plugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
                            swap_plugin PUBLIC ${CMAKE_SOURCE_DIR}/libraries/testing/include
                            swap_plugin PUBLIC ${CMAKE_BINARY_DIR}/unittests/include )
