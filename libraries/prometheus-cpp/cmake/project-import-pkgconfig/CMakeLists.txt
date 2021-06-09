cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(prometheus-cpp-import)

set(CMAKE_CXX_STANDARD 11)

find_package(PkgConfig REQUIRED)

pkg_check_modules(PROMETHEUS_CPP_CORE REQUIRED prometheus-cpp-core)
pkg_check_modules(PROMETHEUS_CPP_PUSH IMPORTED_TARGET prometheus-cpp-push)
pkg_check_modules(PROMETHEUS_CPP_PULL IMPORTED_TARGET prometheus-cpp-pull)

if(PROMETHEUS_CPP_PUSH_FOUND)
  add_executable(sample-client sample_client.cc)
  target_link_libraries(sample-client PRIVATE PkgConfig::PROMETHEUS_CPP_PUSH)
endif()

if(PROMETHEUS_CPP_PULL_FOUND)
  add_executable(sample-server sample_server.cc)
  target_link_libraries(sample-server PRIVATE PkgConfig::PROMETHEUS_CPP_PULL)
endif()
