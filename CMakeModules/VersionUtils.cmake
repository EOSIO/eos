cmake_minimum_required(
  VERSION 3.5 )

function(GENERATE_VERSION_METADATA)
   find_package(Git)
   if(EXISTS ${CMAKE_SOURCE_DIR}/.git AND GIT_FOUND)
      execute_process(
         COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
         OUTPUT_VARIABLE _VERSION_HASH_
         ERROR_QUIET
         OUTPUT_STRIP_TRAILING_WHITESPACE )
      execute_process(
         COMMAND ${GIT_EXECUTABLE} describe --tags --dirty
         OUTPUT_VARIABLE _VERSION_DIRTY_
         ERROR_QUIET
         OUTPUT_STRIP_TRAILING_WHITESPACE )
      string(REGEX MATCH "-dirty$" _VERSION_DIRTY_ ${_VERSION_DIRTY_})
      if("${_VERSION_DIRTY_}" STREQUAL "")
         set(_VERSION_DIRTY_ "false")
      else()
         set(_VERSION_DIRTY_ "true")
      endif()
      set(_VERSION_MAJOR_  ${VERSION_MAJOR}   PARENT_SCOPE)
      set(_VERSION_MINOR_  ${VERSION_MINOR}   PARENT_SCOPE)
      set(_VERSION_PATCH_  ${VERSION_PATCH}   PARENT_SCOPE)
      set(_VERSION_SUFFIX_ ${VERSION_SUFFIX}  PARENT_SCOPE)
      set(_VERSION_HASH_   ${_VERSION_HASH_}  PARENT_SCOPE)
      set(_VERSION_DIRTY_  ${_VERSION_DIRTY_} PARENT_SCOPE)
   else()
      set(_VERSION_MAJOR_  "unknown" PARENT_SCOPE)
      set(_VERSION_MINOR_  ""        PARENT_SCOPE)
      set(_VERSION_PATCH_  ""        PARENT_SCOPE)
      set(_VERSION_SUFFIX_ ""        PARENT_SCOPE)
      set(_VERSION_HASH_   ""        PARENT_SCOPE)
      set(_VERSION_DIRTY_  ""        PARENT_SCOPE)
   endif()
endfunction()
