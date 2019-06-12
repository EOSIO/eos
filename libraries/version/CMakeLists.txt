cmake_minimum_required( 
  VERSION 3.5 )

project(
  Version )

# # The function which grabs the appropriate values for the repository version.
# function(GENERATE_VERSION_METADATA)
#    if(EXISTS ${CMAKE_SOURCE_DIR}/.git AND GIT_FOUND)
#       execute_process(
#          COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
#          OUTPUT_VARIABLE _VERSION_HASH_
#          ERROR_QUIET
#          OUTPUT_STRIP_TRAILING_WHITESPACE )
#       execute_process(
#          COMMAND ${GIT_EXECUTABLE} describe --tags --dirty
#          OUTPUT_VARIABLE _VERSION_DIRTY_
#          ERROR_QUIET
#          OUTPUT_STRIP_TRAILING_WHITESPACE )
#       string(REGEX MATCH "-dirty$" _VERSION_DIRTY_ ${_VERSION_DIRTY_})
#       if("${_VERSION_DIRTY_}" STREQUAL "")
#          set(_VERSION_DIRTY_ "false")
#       else()
#          set(_VERSION_DIRTY_ "true")
#       endif()
#       set(_VERSION_MAJOR_  ${VERSION_MAJOR})
#       set(_VERSION_MINOR_  ${VERSION_MINOR})
#       set(_VERSION_PATCH_  ${VERSION_PATCH})
#       set(_VERSION_SUFFIX_ ${VERSION_SUFFIX})
#       set(_VERSION_HASH_   ${_VERSION_HASH_})
#       set(_VERSION_DIRTY_  ${_VERSION_DIRTY_})
#    else()
#       set(_VERSION_MAJOR_  "unknown")
#       set(_VERSION_MINOR_  "")
#       set(_VERSION_PATCH_  "")
#       set(_VERSION_SUFFIX_ "")
#       set(_VERSION_HASH_   "")
#       set(_VERSION_DIRTY_  "")
#    endif()
# endfunction(GENERATE_VERSION_METADATA)

set(_VERSION_MAJOR_  "unknown" CACHE STRING "Current major version of repository")
set(_VERSION_MINOR_  ""        CACHE STRING "Current minor version of repository")
set(_VERSION_PATCH_  ""        CACHE STRING "Current patch version of repository")
set(_VERSION_SUFFIX_ ""        CACHE STRING "Current suffiex of repository")
set(_VERSION_HASH_   ""        CACHE STRING "Current hash of repository")
set(_VERSION_DIRTY_  ""        CACHE STRING "Current dirty state of repository")

# Construct the library.
add_library(
  version
  "${CMAKE_CURRENT_SOURCE_DIR}/src/version.cpp"
  "${CMAKE_CURRENT_BINARY_DIR}/src/version_impl.cpp" )

# Make dependencies visible.
target_include_directories(
  version
  PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}/include/"
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/" )

# This one runs everytime; save
# Create a custom target to be run upon every build.
message(STATUS ${CMAKE_SOURCE_DIR})
message(STATUS ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Git)
if(EXISTS ${CMAKE_SOURCE_DIR}/.git AND ${GIT_FOUND})
  add_custom_target(
    evaluate_every_build ALL
    COMMAND ${CMAKE_COMMAND} -D_GIT_EXECUTABLE_=${GIT_EXECUTABLE}
                             -D_CMAKE_SOURCE_DIR_="VAR1"
                             -D_CMAKE_CURRENT_SOURCE_DIR_="VAR2"
                             -D_CMAKE_BINARY_DIR_=${CMAKE_BINARY_DIR}
                             -D_CMAKE_CURRENT_BINARY_DIR_=${CMAKE_CURRENT_BINARY_DIR}
                             -P ${CMAKE_SOURCE_DIR}/CMakeModules/VersionUtils.cmake
    COMMENT "***********************Updating version metadata...**********************" VERBATIM )
  # string(REGEX MATCH "-dirty$" _VERSION_DIRTY_ ${_VERSION_DIRTY_})
  # if("${_VERSION_DIRTY_}" STREQUAL "")
  #   set(_VERSION_DIRTY_ "false")
  # else()
  #   set(_VERSION_DIRTY_ "true")
  # endif()
  # set(_VERSION_MAJOR_  ${VERSION_MAJOR}   CACHE STRING "Current major version of repository")
  # set(_VERSION_MINOR_  ${VERSION_MINOR}   CACHE STRING "Current minor version of repository")
  # set(_VERSION_PATCH_  ${VERSION_PATCH}   CACHE STRING "Current patch version of repository")
  # set(_VERSION_SUFFIX_ ${VERSION_SUFFIX}  CACHE STRING "Current suffiex of repository")
  # set(_VERSION_HASH_   ${_VERSION_HASH_}  CACHE STRING "Current hash of repository")
  # set(_VERSION_DIRTY_  ${_VERSION_DIRTY_} CACHE STRING "Current dirty state of repository")
  message(STATUS "--------------------")
  message(STATUS ${_VERSION_MAJOR_})
  message(STATUS "--------------------")
  message(STATUS ${_VERSION_MINOR_})
  message(STATUS "--------------------")
  message(STATUS ${_VERSION_PATCH_})
  message(STATUS "--------------------")
  message(STATUS ${_VERSION_SUFFIX_})
  message(STATUS "--------------------")
  message(STATUS ${_VERSION_HASH_})
  message(STATUS "--------------------")
  message(STATUS ${_VERSION_DIRTY_})
endif()

# This one runs everytime; save
# Create a custom target to be run upon every build.
# find_package(Git)
# if(EXISTS ${CMAKE_SOURCE_DIR}/.git AND ${GIT_FOUND})
#   add_custom_target( evaluate_every_build ALL
#     COMMAND ${CMAKE_COMMAND} -E ${GIT_EXECUTABLE} --version
#     COMMENT "generating........................" VERBATIM )
#   string(REGEX MATCH "-dirty$" _VERSION_DIRTY_ ${_VERSION_DIRTY_})
#   if("${_VERSION_DIRTY_}" STREQUAL "")
#     set(_VERSION_DIRTY_ "false")
#   else()
#     set(_VERSION_DIRTY_ "true")
#   endif()
#   set(_VERSION_MAJOR_  ${VERSION_MAJOR}   CACHE STRING "Current major version of repository")
#   set(_VERSION_MINOR_  ${VERSION_MINOR}   CACHE STRING "Current minor version of repository")
#   set(_VERSION_PATCH_  ${VERSION_PATCH}   CACHE STRING "Current patch version of repository")
#   set(_VERSION_SUFFIX_ ${VERSION_SUFFIX}  CACHE STRING "Current suffiex of repository")
#   set(_VERSION_HASH_   ${_VERSION_HASH_}  CACHE STRING "Current hash of repository")
#   set(_VERSION_DIRTY_  ${_VERSION_DIRTY_} CACHE STRING "Current dirty state of repository")
# endif()

# # Create a custom target to be run upon every build.
# add_custom_target( evaluate_every_build ALL
#    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/CMakeModules/VersionUtils.cmake
#    COMMENT "generating........................" VERBATIM)

# message(STATUS "BEFORE*******************")
# # Create a custom target to be run upon every build.
# find_package(Git)
# if(EXISTS ${CMAKE_SOURCE_DIR}/.git AND GIT_FOUND)
#   message(STATUS "IN*******************")
#   add_custom_target( evaluate_every_build ALL
#     COMMAND 
#   COMMENT "generating........................" VERBATIM)
# message(STATUS "AFTER*******************")
# endif()
# message(STATUS "AFTER IF*******************")

# Install the library in the appropriate places.
install(
  TARGETS version
  RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} )

# Install the header file in the appropriate place.
install(
  DIRECTORY   ${CMAKE_CURRENT_SOURCE_DIR}/include/eosio/version
  DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/eosio/
  FILES_MATCHING PATTERN "*.hpp" )

# Modify and substitute the `.cpp.in` file for a `.cpp` in the build directory.
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/version_impl.cpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/src/version_impl.cpp
  @ONLY )
