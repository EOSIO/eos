
cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
if(POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW) # recognize CMAKE_MSVC_RUNTIME_LIBRARY
endif()

project(prometheus-cpp
  VERSION 0.12.3
  DESCRIPTION "Prometheus Client Library for Modern C++"
  HOMEPAGE_URL "https://github.com/jupp0r/prometheus-cpp"
)

include(GenerateExportHeader)
include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(BUILD_SHARED_LIBS "Build libraries as shared ones" OFF)
option(ENABLE_PULL "Build prometheus-cpp pull library" ON)
option(ENABLE_PUSH "Build prometheus-cpp push library" ON)
option(ENABLE_COMPRESSION "Enable gzip compression" ON)
option(ENABLE_TESTING "Build tests" ON)
option(USE_THIRDPARTY_LIBRARIES "Use 3rdParty submodules" ON)
option(THIRDPARTY_CIVETWEB_WITH_SSL "Enable SSL support for embedded civetweb source code")
option(OVERRIDE_CXX_STANDARD_FLAGS "Force building with -std=c++11 even if the CXXLFAGS are configured differently" ON)
option(GENERATE_PKGCONFIG "Generate and install pkg-config files" ${UNIX})
option(RUN_IWYU "Run include-what-you-use" OFF)

if(OVERRIDE_CXX_STANDARD_FLAGS)
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_EXTENSIONS Off)
endif()

# Set default directory permissions until
# https://gitlab.kitware.com/cmake/cmake/issues/15163
# is fixed
set(CMAKE_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS
    OWNER_READ
    OWNER_WRITE
    OWNER_EXECUTE
    GROUP_READ
    GROUP_EXECUTE
    WORLD_READ
    WORLD_EXECUTE)

# Put DLLs and binaries into same directory
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# Hide things by default for shared libraries
if(BUILD_SHARED_LIBS)
  set(CMAKE_C_VISIBILITY_PRESET hidden)
  set(CMAKE_CXX_VISIBILITY_PRESET hidden)
  set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)
endif()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads)

# include-what-you-use

if(RUN_IWYU)
  find_program(IWYU_EXECUTABLE NAMES include-what-you-use iwyu)
  if(NOT IWYU_EXECUTABLE)
    message(FATAL_ERROR "Include-what-you-use not found")
  endif()

  set(IWYU_ARGS
    "${IWYU_EXECUTABLE}"
    "-Xiwyu" "--no_fwd_decls"
    "-Xiwyu" "--mapping_file=${CMAKE_CURRENT_SOURCE_DIR}/cmake/googletest.imp"
  )

  set(CMAKE_C_INCLUDE_WHAT_YOU_USE ${IWYU_ARGS})
  set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${IWYU_ARGS})
endif()

# check for required libatomic

include(CheckAtomic)

if(ENABLE_TESTING)
  if(USE_THIRDPARTY_LIBRARIES)
    find_package(googlemock-3rdparty CONFIG REQUIRED)
  else()
    find_package(GTest 1.8.1 CONFIG REQUIRED)
  endif()
  find_package(benchmark CONFIG)
  enable_testing()
endif()

# build flags for CI system

if(ENABLE_WARNINGS_AS_ERRORS AND NOT MSVC)
  add_compile_options(
    $<$<AND:$<STREQUAL:$<COMPILE_LANGUAGE>,CXX>,$<CXX_COMPILER_ID:AppleClang>>:-Werror>
    $<$<AND:$<STREQUAL:$<COMPILE_LANGUAGE>,CXX>,$<CXX_COMPILER_ID:AppleClang>>:-Wall>
    $<$<AND:$<STREQUAL:$<COMPILE_LANGUAGE>,CXX>,$<CXX_COMPILER_ID:AppleClang>>:-Wextra>
    $<$<AND:$<STREQUAL:$<COMPILE_LANGUAGE>,CXX>,$<CXX_COMPILER_ID:AppleClang>>:-pedantic-errors>
    $<$<AND:$<STREQUAL:$<COMPILE_LANGUAGE>,CXX>,$<CXX_COMPILER_ID:GNU>>:-Werror>
    $<$<AND:$<STREQUAL:$<COMPILE_LANGUAGE>,CXX>,$<CXX_COMPILER_ID:GNU>>:-Wall>
    $<$<AND:$<STREQUAL:$<COMPILE_LANGUAGE>,CXX>,$<CXX_COMPILER_ID:GNU>>:-pedantic-errors>
  )
endif()

# prometheus-cpp

add_subdirectory(core)

if(ENABLE_PULL)
  add_subdirectory(pull)
endif()

if(ENABLE_PUSH)
  add_subdirectory(push)
endif()

# install

include(CMakePackageConfigHelpers)

install(
  EXPORT ${PROJECT_NAME}-targets
  NAMESPACE ${PROJECT_NAME}::
  FILE ${PROJECT_NAME}-targets.cmake
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}-config.cmake.in"
  ${PROJECT_NAME}-config.cmake
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
  PATH_VARS CMAKE_INSTALL_INCLUDEDIR
)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)

# packaging

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(CPACK_PACKAGE_CONTACT "prometheus-cpp@@noreply.github.com")
  set(CPACK_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
  set(CPACK_PACKAGE_RELOCATABLE OFF)
  set(CPACK_PACKAGE_VENDOR "The prometheus-cpp authors")

  if(CMAKE_VERSION VERSION_LESS "3.12")
    set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
    set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
    set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
  endif()

  set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
  set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

  set(CPACK_RPM_PACKAGE_AUTOREQPROV ON)
  set(CPACK_RPM_FILE_NAME RPM-DEFAULT)

  include(CPack)
endif()

# summary

include(FeatureSummary)
add_feature_info("Pull" "${ENABLE_PULL}" "support for pulling metrics")
add_feature_info("Push" "${ENABLE_PUSH}" "support for pushing metrics to a push-gateway")
add_feature_info("Compression" "${ENABLE_COMPRESSION}" "support for zlib compression of metrics")
add_feature_info("pkg-config" "${GENERATE_PKGCONFIG}" "generate pkg-config files")
add_feature_info("IYWU" "${RUN_IWYU}" "include-what-you-use")
feature_summary(WHAT ALL)
