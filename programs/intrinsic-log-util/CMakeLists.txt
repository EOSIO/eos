add_executable( intrinsic-log-util
   main.cpp
   print.cpp
   diff.cpp
)

if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling intrinsic-log-util with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

target_include_directories(intrinsic-log-util PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries( intrinsic-log-util
        PRIVATE appbase
        PRIVATE eosio_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

copy_bin( intrinsic-log-util )
#install( TARGETS
#   intrinsic-log-util
#
#   RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
#   LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
#   ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
#)
