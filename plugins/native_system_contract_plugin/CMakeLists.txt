file(GLOB HEADERS "include/eos/native_system_contract_plugin/*.hpp")
add_library( native_system_contract_plugin
             native_system_contract_plugin.cpp
             ${HEADERS} )

target_link_libraries( native_system_contract_plugin chain_plugin appbase fc )
target_include_directories( native_system_contract_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   native_system_contract_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/eos/native_system_contract_plugin" )
