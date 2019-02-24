add_executable(create-genesis main.cpp golos_objects.cpp)

if(UNIX AND NOT APPLE)
  set(rt_library rt)
endif()

find_package(Gperftools QUIET)
if(GPERFTOOLS_FOUND)
  message(STATUS "Found gperftools; compiling create-genesis with TCMalloc")
  list(APPEND PLATFORM_SPECIFIC_LIBS tcmalloc)
endif()

target_include_directories(create-genesis PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(create-genesis
  PRIVATE appbase
  PRIVATE eosio_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS}
)

install(TARGETS
  create-genesis

  RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)
