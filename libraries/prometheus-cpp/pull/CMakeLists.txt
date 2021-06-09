if(USE_THIRDPARTY_LIBRARIES)
  find_package(civetweb-3rdparty CONFIG REQUIRED)
  add_library(${PROJECT_NAME}::civetweb ALIAS civetweb)
  install(
    TARGETS civetweb
    EXPORT ${PROJECT_NAME}-targets
    # keep embedded civetweb headers scoped to prometheus(-cpp)
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/prometheus
  )
else()
  find_package(civetweb CONFIG REQUIRED)

  # work-around https://github.com/civetweb/civetweb/pull/918
  if(WIN32 AND NOT TARGET WINSOCK::WINSOCK)
    add_library(WINSOCK::WINSOCK INTERFACE IMPORTED)
    target_link_libraries(WINSOCK::WINSOCK INTERFACE ws2_32)
  endif()
endif()

if(ENABLE_COMPRESSION)
  find_package(ZLIB REQUIRED)
endif()

add_library(pull
  src/basic_auth.cc
  src/basic_auth.h
  src/endpoint.cc
  src/endpoint.h
  src/exposer.cc
  src/handler.cc
  src/handler.h
  src/metrics_collector.cc
  src/metrics_collector.h

  src/detail/base64.h
)

add_library(${PROJECT_NAME}::pull ALIAS pull)

target_link_libraries(pull
  PUBLIC
    ${PROJECT_NAME}::core
  PRIVATE
    Threads::Threads
    $<IF:$<BOOL:${USE_THIRDPARTY_LIBRARIES}>,${PROJECT_NAME}::civetweb,civetweb::civetweb-cpp>
    $<$<AND:$<BOOL:UNIX>,$<NOT:$<BOOL:APPLE>>>:rt>
    $<$<BOOL:${ENABLE_COMPRESSION}>:ZLIB::ZLIB>
)

target_include_directories(pull
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  PRIVATE
    ${CIVETWEB_INCLUDE_DIRS}
)

target_compile_definitions(pull
  PRIVATE
    $<$<BOOL:${ENABLE_COMPRESSION}>:HAVE_ZLIB>
)

set_target_properties(pull
  PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}-pull
    DEFINE_SYMBOL PROMETHEUS_CPP_PULL_EXPORTS
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
)

generate_export_header(pull
  BASE_NAME ${PROJECT_NAME}-pull
  EXPORT_FILE_NAME include/prometheus/detail/pull_export.h
)

install(
  TARGETS pull
  EXPORT ${PROJECT_NAME}-targets
  RUNTIME DESTINATION  ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION  ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION  ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
  DIRECTORY include/ ${CMAKE_CURRENT_BINARY_DIR}/include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(GENERATE_PKGCONFIG)
  set(PKGCONFIG_LIBS)
  set(PKGCONFIG_REQUIRES)

  if(NOT USE_THIRDPARTY_LIBRARIES)
    string(APPEND PKGCONFIG_LIBS " -lcivetweb-cpp -lcivetweb")
  endif()

  if(ENABLE_COMPRESSION)
    string(APPEND PKGCONFIG_REQUIRES " zlib")
  endif()

  configure_file(
    ${PROJECT_SOURCE_DIR}/cmake/prometheus-cpp-pull.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/prometheus-cpp-pull.pc
    @ONLY
  )

  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/prometheus-cpp-pull.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
  )
endif()

if(ENABLE_TESTING)
  add_library(pull_internal_headers INTERFACE)
  add_library(${PROJECT_NAME}::pull_internal_headers ALIAS pull_internal_headers)
  target_include_directories(pull_internal_headers INTERFACE src)

  add_subdirectory(tests)
endif()
