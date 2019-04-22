file(GLOB HEADERS "include/eosio/pbft_plugin/*.hpp")
add_library( pbft_plugin
        pbft_plugin.cpp
             ${HEADERS} )

target_link_libraries( pbft_plugin appbase fc eosio_chain chain_plugin net_plugin)
target_include_directories( pbft_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../chain_interface/include")
