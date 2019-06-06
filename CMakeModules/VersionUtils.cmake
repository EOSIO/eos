function(GENERATE_VERSION_METADATA)
   set(_VERSION_MAJOR_  ${VERSION_MAJOR}  PARENT_SCOPE)
   set(_VERSION_MINOR_  ${VERSION_MINOR}  PARENT_SCOPE)
   set(_VERSION_PATCH_  ${VERSION_PATCH}  PARENT_SCOPE)
   set(_VERSION_SUFFIX_ ${VERSION_SUFFIX} PARENT_SCOPE)

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
       # if dirty empty -> false
       # else -> true
       #
   endif()
   set(_VERSION_HASH_   ${_VERSION_HASH_}  PARENT_SCOPE)
   set(_VERSION_DIRTY_  ${_VERSION_DIRTY_} PARENT_SCOPE)
   message(STATUS "-------begin-------")
   message(STATUS ${_VERSION_MAJOR_})
   message(STATUS ${_VERSION_MINOR_})
   message(STATUS ${_VERSION_PATCH_})
   message(STATUS ${_VERSION_SUFFIX_})
   message(STATUS ${_VERSION_HASH_})
   message(STATUS ${_VERSION_DIRTY_})
   message(STATUS "-------end-------")
endfunction()
