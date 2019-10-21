add_executable(launcher-service main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

target_include_directories(launcher-service PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(launcher-service
                      PRIVATE appbase version chain_api_plugin producer_plugin chain_plugin http_plugin eosio_chain launcher_service_plugin launcher_service_api_plugin fc Boost::program_options ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   launcher-service RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
)

