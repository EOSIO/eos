file(GLOB HEADERS "include/eosio/net_plugin/*.hpp")
add_library( net_plugin
             net_plugin.cpp
             ${HEADERS} )

target_link_libraries( net_plugin chain_plugin producer_plugin appbase fc )
target_include_directories( net_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   net_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/eosio/net_plugin" )
