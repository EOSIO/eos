add_executable( ${KEY_STORE_EXECUTABLE_NAME} main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

configure_file(config.hpp.in config.hpp ESCAPE_QUOTES)

target_link_libraries( ${KEY_STORE_EXECUTABLE_NAME}
        PRIVATE appbase version
        PRIVATE wallet_api_plugin wallet_plugin
        PRIVATE http_plugin
        PRIVATE eosio_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
target_include_directories(${KEY_STORE_EXECUTABLE_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

mas_sign(${KEY_STORE_EXECUTABLE_NAME})

copy_bin( ${KEY_STORE_EXECUTABLE_NAME} )
install( TARGETS
   ${KEY_STORE_EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR} COMPONENT base
)
