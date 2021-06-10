configure_file(help_text.cpp.in help_text.cpp @ONLY)
configure_file(config.hpp.in config.hpp ESCAPE_QUOTES)
add_executable( ${CLI_CLIENT_EXECUTABLE_NAME} main.cpp httpc.cpp "${CMAKE_CURRENT_BINARY_DIR}/help_text.cpp" localize.hpp "${CMAKE_CURRENT_BINARY_DIR}/config.hpp" CLI11.hpp)
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

set(LOCALEDIR ${CMAKE_INSTALL_PREFIX}/share/locale)
set(LOCALEDOMAIN ${CLI_CLIENT_EXECUTABLE_NAME})

target_include_directories(${CLI_CLIENT_EXECUTABLE_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries( ${CLI_CLIENT_EXECUTABLE_NAME}
                       PRIVATE appbase version chain_api_plugin producer_plugin chain_plugin http_plugin eosio_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )


copy_bin( ${CLI_CLIENT_EXECUTABLE_NAME} )
install( TARGETS
   ${CLI_CLIENT_EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR} COMPONENT base
)
