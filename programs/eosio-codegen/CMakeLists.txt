add_executable( eosio-codegen main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling eosio-codegen with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

target_link_libraries( eosio-codegen
                       PRIVATE fc eosio_chain ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   eosio-codegen

   RUNTIME DESTINATION usr/bin
   LIBRARY DESTINATION usr/lib
   ARCHIVE DESTINATION usr/lib
)
