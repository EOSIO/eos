add_executable( eosd main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling steemd with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

#link_directories(/Users/newworld/anaconda/lib)

add_library(python3.6m SHARED IMPORTED)
 set_property(TARGET python3.6m PROPERTY IMPORTED_LOCATION "/Users/newworld/anaconda/lib/libpython3.6m.so")


target_link_libraries( eosd
                       PRIVATE appbase account_history_api_plugin account_history_plugin chain_api_plugin producer_plugin chain_plugin net_plugin http_plugin python3.6m py_plugin eos_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   eosd

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
