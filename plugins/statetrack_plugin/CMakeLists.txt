list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

## load in pkg-config support
find_package(PkgConfig REQUIRED)
## use pkg-config to get hints for 0mq locations
pkg_check_modules(PC_ZeroMQ REQUIRED libzmq)

## use the hint from above to find where 'zmq.hpp' is located
find_path(ZeroMQ_INCLUDE_DIR
  NAMES zmq.hpp
  PATHS ${PC_ZeroMQ_INCLUDE_DIRS}
)

## use the hint from about to find the location of libzmq
find_library(ZeroMQ_LIBRARY
  NAMES zmq
  PATHS ${PC_ZeroMQ_LIBRARY_DIRS}
  )

message(STATUS "[Additional Plugin] EOSIO ZeroMQ plugin enabled")

file(GLOB HEADERS "include/eosio/statetrack_plugin/*.hpp")
add_library( statetrack_plugin
             statetrack_plugin.cpp
             statetrack_plugin_impl.cpp
             ${HEADERS} )

target_link_libraries( statetrack_plugin ${ZeroMQ_LIBRARY} chain_plugin appbase fc )
target_include_directories( statetrack_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
