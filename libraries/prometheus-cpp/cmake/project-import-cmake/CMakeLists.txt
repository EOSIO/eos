cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(prometheus-cpp-import)

set(CMAKE_CXX_STANDARD 11)

find_package(prometheus-cpp CONFIG REQUIRED)

if(PROMETHEUS_CPP_ENABLE_PUSH)
  add_executable(sample-client sample_client.cc)
  target_link_libraries(sample-client PRIVATE prometheus-cpp::push)
endif()

if(PROMETHEUS_CPP_ENABLE_PULL)
  add_executable(sample-server sample_server.cc)
  target_link_libraries(sample-server PRIVATE prometheus-cpp::pull)
endif()
