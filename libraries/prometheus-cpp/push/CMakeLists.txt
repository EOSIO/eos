
find_package(CURL REQUIRED)

add_library(push
  src/gateway.cc
)

add_library(${PROJECT_NAME}::push ALIAS push)

target_link_libraries(push
  PUBLIC
    ${PROJECT_NAME}::core
  PRIVATE
    Threads::Threads
    CURL::libcurl
    $<$<AND:$<BOOL:UNIX>,$<NOT:$<BOOL:APPLE>>>:rt>
)

target_include_directories(push
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
)

set_target_properties(push
  PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}-push
    DEFINE_SYMBOL PROMETHEUS_CPP_PUSH_EXPORTS
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
)

generate_export_header(push
  BASE_NAME ${PROJECT_NAME}-push
  EXPORT_FILE_NAME include/prometheus/detail/push_export.h
)

install(
  TARGETS push
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

  string(APPEND PKGCONFIG_REQUIRES " libcurl")

  configure_file(
    ${PROJECT_SOURCE_DIR}/cmake/prometheus-cpp-push.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/prometheus-cpp-push.pc
    @ONLY
  )

  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/prometheus-cpp-push.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
  )
endif()

if(ENABLE_TESTING)
  add_subdirectory(tests)
endif()
