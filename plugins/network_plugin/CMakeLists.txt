file(GLOB HEADERS "include/eos/network_plugin/*.hpp")
add_library( network_plugin
             network_plugin.cpp
             ${HEADERS} )

target_link_libraries( network_plugin chain_plugin appbase fc )
target_include_directories( network_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   network_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/eos/network_plugin" )
