add_executable( eosiod main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling eosiod with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

target_link_libraries( eosiod
        PRIVATE appbase
#        PRIVATE account_history_api_plugin account_history_plugin db_plugin
        PRIVATE chain_api_plugin producer_plugin chain_plugin 
       PRIVATE  wallet_api_plugin
#        PRIVATE net_plugin 
        PRIVATE http_plugin
        PRIVATE eosio_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   eosiod

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
