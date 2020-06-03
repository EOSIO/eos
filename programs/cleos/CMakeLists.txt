configure_file(help_text.cpp.in help_text.cpp @ONLY)
add_executable( ${CLI_CLIENT_EXECUTABLE_NAME} main.cpp httpc.cpp ${CMAKE_CURRENT_BINARY_DIR}/help_text.cpp localize.hpp config.hpp CLI11.hpp)
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

find_package(Intl REQUIRED)

set(LOCALEDIR ${CMAKE_INSTALL_PREFIX}/share/locale)
set(LOCALEDOMAIN ${CLI_CLIENT_EXECUTABLE_NAME})
configure_file(config.hpp.in config.hpp ESCAPE_QUOTES)

target_include_directories(${CLI_CLIENT_EXECUTABLE_NAME} PUBLIC ${Intl_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries( ${CLI_CLIENT_EXECUTABLE_NAME}
                       PRIVATE appbase version chain_api_plugin producer_plugin chain_plugin http_plugin eosio_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} ${Intl_LIBRARIES} )


copy_bin( ${CLI_CLIENT_EXECUTABLE_NAME} )
install( TARGETS
   ${CLI_CLIENT_EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR} COMPONENT base
)
