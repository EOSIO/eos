add_executable( eosio-blocklog main.cpp )

if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

target_include_directories(eosio-blocklog PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries( eosio-blocklog
        PRIVATE appbase
        PRIVATE eosio_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

copy_bin( eosio-blocklog )
install( TARGETS
   eosio-blocklog

   COMPONENT base

   RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
   LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
   ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)
