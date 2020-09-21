file(GLOB HEADERS "include/eosio/blockvault/*.hpp")
add_library( blockvault
             blockvault.cpp
             ${HEADERS} )

target_link_libraries( blockvault appbase fc http_plugin chain_plugin eosio_testing )
target_include_directories( blockvault PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
