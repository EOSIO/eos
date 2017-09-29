file(GLOB HEADERS "include/eos/db_plugin/*.hpp")
add_library( db_plugin
             db_plugin.cpp
             ${HEADERS} )

target_link_libraries( db_plugin chain_plugin eos_chain appbase )
target_include_directories( db_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   db_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/eos/db_plugin" )
