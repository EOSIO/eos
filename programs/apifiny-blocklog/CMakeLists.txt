add_executable( apifiny-blocklog main.cpp )

if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

target_include_directories(apifiny-blocklog PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries( apifiny-blocklog
        PRIVATE appbase
        PRIVATE apifiny_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

copy_bin( apifiny-blocklog )
install( TARGETS
   apifiny-blocklog

   COMPONENT base

   RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
   LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
   ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)
