file(GLOB HEADERS "include/eos/tcp_connection_plugin/*.hpp")
add_library( tcp_connection_plugin
             tcp_connection_plugin.cpp
             tcp_connection_initiator.cpp
             tcp_connection.cpp
             ${HEADERS} )

target_link_libraries( tcp_connection_plugin network_plugin appbase fc )
target_include_directories( tcp_connection_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   tcp_connection_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/eos/tcp_connection_plugin" )
