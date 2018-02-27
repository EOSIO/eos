file(GLOB HEADERS "include/eosio/debug_plugin/*.hpp")
add_library( debug_plugin
             debug_plugin.cpp
             ${HEADERS} )

target_link_libraries( debug_plugin appbase fc chain_plugin eosio_chain )
target_include_directories( debug_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   debug_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/eosio/debug_plugin" )
