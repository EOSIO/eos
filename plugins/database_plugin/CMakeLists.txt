file(GLOB HEADERS "include/eos/database_plugin/*.hpp")
add_library( database_plugin
             database_plugin.cpp
             ${HEADERS} )

target_link_libraries( database_plugin appbase chainbase fc )
target_include_directories( database_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   database_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/eos/database_plugin" )
