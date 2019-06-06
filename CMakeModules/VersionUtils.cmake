function(GENERATE_VERSION_METADATA)
   set(_VERSION_MAJOR_  ${VERSION_MAJOR})
   set(_VERSION_MINOR_  ${VERSION_MINOR})
   set(_VERSION_PATCH_  ${VERSION_PATCH})
   set(_VERSION_SUFFIX_ ${VERSION_SUFFIX})
   set(_VERSION_HASH_   ${})
   set(_VERSION_DIRTY_  ${})

   find_package(Git)
   if(EXISTS ${CMAKE_SOURCE_DIR}/.git AND GIT_FOUND)
      execute_process(
         COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 HEAD
         OUTPUT_VARIABLE _VERSION_HASH_
         ERROR_QUIET
         OUTPUT_STRIP_TRAILING_WHITESPACE )
      execute_process(
         COMMAND ${GIT_EXECUTABLE} describe --tags --dirty
         OUTPUT_VARIABLE _VERSION_DIRTY_
         ERROR_QUIET
         OUTPUT_STRIP_TRAILING_WHITESPACE )
      string(REGEX MATCH "-dirty$" _VERSION_DIRTY_ ${_VERSION_DIRTY_})
      # configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/version.hpp @ONLY)
   endif()
endfunction()
