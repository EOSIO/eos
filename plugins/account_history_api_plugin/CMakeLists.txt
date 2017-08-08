file(GLOB HEADERS "include/eos/account_history_api_plugin/*.hpp")
add_library( account_history_api_plugin
             account_history_api_plugin.cpp
             ${HEADERS} )

target_link_libraries( account_history_api_plugin account_history_plugin chain_plugin http_plugin appbase )
target_include_directories( account_history_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   account_history_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/eos/account_history_api_plugin" )
