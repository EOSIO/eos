file(GLOB HEADERS "include/eosio/template_plugin/*.hpp")
add_library( template_plugin
             template_plugin.cpp
             ${HEADERS} )

target_link_libraries( template_plugin appbase fc )
target_include_directories( template_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   template_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/eosio/template_plugin" )
