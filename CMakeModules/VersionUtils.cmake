# cmake_minimum_required(
#   VERSION 3.5 )

# find_package(Git)
# function(GENERATE_VERSION_METADATA)
   # message(STATUS "---------------------VersionUtils before if----------------------------------")
   # find_package(Git)
   # message(STATUS ${GIT_FOUND})
   # message(STATUS ${CMAKE_SOURCE_DIR})
   # if(EXISTS ${CMAKE_SOURCE_DIR}/.git AND GIT_FOUND)
   message(STATUS ${_CMAKE_SOURCE_DIR_})
   message(STATUS ${_CMAKE_CURRENT_SOURCE_DIR_})
      message(STATUS "-----------------VersionUtils in if----------------------------------")
      execute_process(
         COMMAND ${_GIT_EXECUTABLE_} rev-parse HEAD
         OUTPUT_VARIABLE _VERSION_HASH_
         ERROR_QUIET
         OUTPUT_STRIP_TRAILING_WHITESPACE )
      execute_process(
         COMMAND ${_GIT_EXECUTABLE_} describe --tags --dirty
         OUTPUT_VARIABLE _VERSION_DIRTY_
         ERROR_QUIET
         OUTPUT_STRIP_TRAILING_WHITESPACE )
       string(REGEX MATCH "-dirty$" _VERSION_DIRTY_ ${_VERSION_DIRTY_})
       
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
      if("${_VERSION_DIRTY_}" STREQUAL "")
         set(_VERSION_TEMP_ "false" CACHE STRING "Current dirty state of repository" FORCE)
      else()
         set(_VERSION_TEMP_ "true" CACHE STRING "Current dirty state of repository" FORCE)
      endif()
      # set(_VERSION_MAJOR_  ${VERSION_MAJOR})
      # set(_VERSION_MINOR_  ${VERSION_MINOR})
      # set(_VERSION_PATCH_  ${VERSION_PATCH})
      # set(_VERSION_SUFFIX_ ${VERSION_SUFFIX})
      # set(_VERSION_HASH_   ${_VERSION_HASH_})
      # set(_VERSION_DIRTY_  ${_VERSION_DIRTY_})
   # else()
   #    set(_VERSION_MAJOR_  "unknown")
   #    set(_VERSION_MINOR_  "")
   #    set(_VERSION_PATCH_  "")
   #    set(_VERSION_SUFFIX_ "")
   #    set(_VERSION_HASH_   "")
   #    set(_VERSION_DIRTY_  "")
   # endif()
   message(STATUS "--------------VersionUtils after if----------------------------------") 
# endfunction(GENERATE_VERSION_METADATA)
