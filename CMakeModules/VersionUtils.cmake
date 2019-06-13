cmake_minimum_required(VERSION 3.5)

function(GENERATE_VERSION_METADATA)
   # Execute `git` to grab the corresponding data.
   execute_process(
      COMMAND ${GIT_EXEC} rev-parse HEAD
      WORKING_DIRECTORY ${SRC_DIR}
      OUTPUT_VARIABLE V_HASH
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE )
   execute_process(
      COMMAND ${GIT_EXEC} describe --tags --dirty
      WORKING_DIRECTORY ${SRC_DIR}
      OUTPUT_VARIABLE V_DIRTY
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE )

   # Look for the substring "-dirty" in the variable `_VERSION_DIRTY_`.
   string(REGEX MATCH "-dirty$" V_DIRTY ${V_DIRTY})

   # If `_VERSION_DIRTY_` is empty, we know that the repository isn't dirty and vice versa.
   if("${V_DIRTY}" STREQUAL "")
      set(V_DIRTY "false")
   else()
      set(V_DIRTY "true")
   endif()

   # Define the proper version metadata for the file `version_impl.cpp.in`.
   set(_VERSION_MAJOR_  ${V_MAJOR})
   set(_VERSION_MINOR_  ${V_MINOR})
   set(_VERSION_PATCH_  ${V_PATCH})
   set(_VERSION_SUFFIX_ ${V_SUFFIX})
   set(_VERSION_HASH_   ${V_HASH})
   set(_VERSION_DIRTY_  ${V_DIRTY})
   
   # Modify and substitute the `.cpp.in` file for a `.cpp` in the build directory.
   configure_file(
      ${CUR_SRC_DIR}/src/version_impl.cpp.in
      ${CUR_BIN_DIR}/src/version_impl.cpp
      @ONLY )
endfunction(GENERATE_VERSION_METADATA)
  
GENERATE_VERSION_METADATA()
