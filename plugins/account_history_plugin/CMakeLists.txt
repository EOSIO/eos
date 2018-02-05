file(GLOB HEADERS "include/eosio/account_history_plugin/*.hpp")
add_library( account_history_plugin
             account_history_plugin.cpp
             ${HEADERS} )

target_link_libraries( account_history_plugin chain_plugin eosio_chain appbase )
target_include_directories( account_history_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   account_history_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/eos/account_history_plugin" )
