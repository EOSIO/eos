add_executable(create-genesis-ee main.cpp genesis_ee_builder.cpp event_engine_genesis.cpp)

if(UNIX AND NOT APPLE)
  set(rt_library rt)
endif()

find_package(Gperftools QUIET)
if(GPERFTOOLS_FOUND)
  message(STATUS "Found gperftools; compiling create-genesis-ee with TCMalloc")
  list(APPEND PLATFORM_SPECIFIC_LIBS tcmalloc)
endif()

target_include_directories(create-genesis-ee PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(create-genesis-ee
  PRIVATE appbase
  PRIVATE chainbase
  PRIVATE eosio_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS}
)

install(TARGETS
  create-genesis-ee

  RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)
