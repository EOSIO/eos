cmake_minimum_required(
  VERSION 3.5 )

function(GENERATE_VERSION_METADATA)
   # Execute `git` to grab the corresponding data.
   message(STATUS "*********************************")
   message(STATUS ${CMAKE_CURRENT_SOURCE_DIR})
   message(STATUS "*********************************")
   message(STATUS ${ROOT_DIR})
   message(STATUS "*********************************")
   message(STATUS ${GIT_EXEC})
   message(STATUS "*********************************")
   message(STATUS ${LIB_BIN_DIR})
   message(STATUS "*********************************")
   message(STATUS ${LIB_CUR_DIR})
   message(STATUS "*********************************")
   message(STATUS ${V_MAJOR})
   message(STATUS "*********************************")
   message(STATUS ${V_MINOR})
   message(STATUS "*********************************")
   message(STATUS ${V_PATCH})
   message(STATUS "*********************************")
   message(STATUS ${V_SUFFIX})
   message(STATUS "*********************************")
   execute_process(
      COMMAND ${GIT_EXEC} rev-parse HEAD
      WORKING_DIRECTORY ${ROOT_DIR}
      OUTPUT_VARIABLE _VERSION_HASH_
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE )
   execute_process(
      COMMAND ${GIT_EXEC} describe --tags --dirty
      WORKING_DIRECTORY ${ROOT_DIR}
      OUTPUT_VARIABLE _VERSION_DIRTY_
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE )

   message(STATUS "*********************************")
   message(STATUS ${_VERSION_HASH_})
   message(STATUS "*********************************")
   message(STATUS ${_VERSION_DIRTY_})
   message(STATUS "*********************************")
    
   string(REGEX MATCH "-dirty$" _VERSION_DIRTY_ ${_VERSION_DIRTY_})
    
   if("${_VERSION_DIRTY_}" STREQUAL "")
      set(_VERSION_DIRTY_ "false")
   else()
      set(_VERSION_DIRTY_ "true")
   endif()

   # message(STATUS "*********************************")
   # message(STATUS ${V_MAJOR})
   # message(STATUS "*********************************")
   # message(STATUS ${V_MINOR})
   # message(STATUS "*********************************")
   # message(STATUS ${V_PATCH})
   # message(STATUS "*********************************")
   # message(STATUS ${V_SUFFIX})
   # message(STATUS "*********************************")
    
   set(_VERSION_MAJOR_  ${V_MAJOR})
   set(_VERSION_MINOR_  ${V_MINOR})
   set(_VERSION_PATCH_  ${V_PATCH})
   set(_VERSION_SUFFIX_ ${V_SUFFIX})
   set(_VERSION_HASH_   ${_VERSION_HASH_})
   set(_VERSION_DIRTY_  ${_VERSION_DIRTY_})

   # message(STATUS "*********************************")
   # message(STATUS ${_VERSION_MAJOR_})
   # message(STATUS "*********************************")
   # message(STATUS ${_VERSION_MINOR_})
   # message(STATUS "*********************************")
   # message(STATUS ${_VERSION_PATCH_})
   # message(STATUS "*********************************")
   # message(STATUS ${_VERSION_SUFFIX_})
   # message(STATUS "*********************************")
    
   # Modify and substitute the `.cpp.in` file for a `.cpp` in the build directory.
   configure_file(
      ${LIB_CUR_DIR}/src/version_impl.cpp.in
      ${LIB_BIN_DIR}/src/version_impl.cpp
      @ONLY )
endfunction(GENERATE_VERSION_METADATA)

GENERATE_VERSION_METADATA()
