file(GLOB HEADERS "include/eosio/blockvault_client_plugin/*.hpp")
add_library( blockvault_client_plugin
             blockvault_client_plugin.cpp
             ${HEADERS} )

target_link_libraries( blockvault_client_plugin appbase fc http_plugin chain_plugin eosio_testing )
target_include_directories( blockvault_client_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
