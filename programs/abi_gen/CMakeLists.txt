set(SOURCES main.cpp)
find_package(LLVM 4.0 REQUIRED CONFIG)

link_directories(${LLVM_LIBRARY_DIR})

add_executable(abi_gen ${SOURCES})

set( CMAKE_CXX_STANDARD 14 )

if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

target_link_libraries(abi_gen abi_generator)


install( TARGETS
   abi_gen
   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
