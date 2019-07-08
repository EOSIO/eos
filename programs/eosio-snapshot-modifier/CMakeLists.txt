add_executable( eosio-snapshot-modifier main.cpp )

if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling eosio-snapshot-modifier with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

target_include_directories(eosio-snapshot-modifier PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries( eosio-snapshot-modifier
        PRIVATE chain_plugin 
        PRIVATE eosio_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

copy_bin( eosio-snapshot-modifier )
install( TARGETS
   eosio-snapshot-modifier

   RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
   LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
   ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)
