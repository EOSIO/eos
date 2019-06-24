file(GLOB HEADERS "include/eosio/topology_plugin/*.hpp" )
add_library( topology_plugin
             topology_plugin.cpp
             ${HEADERS} )

target_link_libraries( topology_plugin net_plugin producer_plugin appbase fc )
target_include_directories( topology_plugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/../chain_interface/include  ${CMAKE_CURRENT_SOURCE_DIR}/../net_plugin/include ${CMAKE_CURRENT_SOURCE_DIR}/../../libraries/appbase/include ${CMAKE_CURRENT_SOURCE_DIR}/../../libraries/chain/include)
