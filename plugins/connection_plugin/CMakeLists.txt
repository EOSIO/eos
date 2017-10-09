file(GLOB HEADERS "include/eos/connection_plugin/*.hpp")
add_library( connection_plugin
             connection_plugin.cpp
             ${HEADERS} )

target_link_libraries( connection_plugin network_plugin appbase fc )
target_include_directories( connection_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   connection_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/eos/connection_plugin" )
