
add_library(core
  src/check_names.cc
  src/counter.cc
  src/detail/builder.cc
  src/detail/ckms_quantiles.cc
  src/detail/time_window_quantiles.cc
  src/detail/utils.cc
  src/family.cc
  src/gauge.cc
  src/histogram.cc
  src/registry.cc
  src/serializer.cc
  src/summary.cc
  src/text_serializer.cc
)

add_library(${PROJECT_NAME}::core ALIAS core)

target_link_libraries(core
  PRIVATE
    Threads::Threads
    $<$<AND:$<BOOL:UNIX>,$<NOT:$<BOOL:APPLE>>>:rt>
)

if(HAVE_CXX_LIBATOMIC)
  # the exported library config must use libatomic unconditionally
  # (the HAVE_CXX_LIBATOMIC variable should not leak into the target config)
  target_link_libraries(core PUBLIC atomic)
endif()

target_include_directories(core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
)

set_target_properties(core
  PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}-core
    DEFINE_SYMBOL PROMETHEUS_CPP_CORE_EXPORTS
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
)

generate_export_header(core
  BASE_NAME ${PROJECT_NAME}-core
  EXPORT_FILE_NAME include/prometheus/detail/core_export.h
)

install(
  TARGETS core
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

  if(HAVE_CXX_LIBATOMIC)
    string(APPEND PKGCONFIG_LIBS " -latomic")
  endif()

  configure_file(
    ${PROJECT_SOURCE_DIR}/cmake/prometheus-cpp-core.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/prometheus-cpp-core.pc
    @ONLY
  )

  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/prometheus-cpp-core.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
  )
endif()

if(ENABLE_TESTING)
  add_subdirectory(tests)
endif()

if(benchmark_FOUND)
  add_subdirectory(benchmarks)
endif()
