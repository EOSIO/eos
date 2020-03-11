cmake_minimum_required(VERSION 3.5)
project(EosioVersion)

# Construct the library target.
add_library(
   eosio_version
   "${CMAKE_CURRENT_BINARY_DIR}/src/version.cpp"
   )

# Make dependencies visible to the given target library to be constructed.
target_include_directories(
   eosio_version
   PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}/include/"
   PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/" )

# Create a custom target to update the version metadata upon every build.
find_package(Git)
if(EXISTS ${CMAKE_SOURCE_DIR}/.git AND ${GIT_FOUND})
   add_custom_target(
      evaluate_every_build ALL
      COMMAND ${CMAKE_COMMAND} -DGIT_EXEC=${GIT_EXECUTABLE}
       -DCURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
       -DCURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
       -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
       -D_VERSION_MAJOR_=${VERSION_MAJOR}
       -D_VERSION_MINOR_=${VERSION_MINOR}
       -D_VERSION_PATCH_=${VERSION_PATCH}
       -D_VERSION_SUFFIX_=${VERSION_SUFFIX}
       -P ${CMAKE_CURRENT_SOURCE_DIR}/version_metadata.cmake
      BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/src/version.cpp
      COMMENT "Updating version metadata..." VERBATIM )

      # Create a dependency for the given library target.
      add_dependencies(eosio_version evaluate_every_build)
else()
   set(_VERSION_MAJOR_  "-1")
   # Modify and substitute the `.cpp.in` file for a `.cpp` in the build directory.
   configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/src/version.cpp.in
      ${CMAKE_CURRENT_BINARY_DIR}/src/version.cpp
      @ONLY )
endif()
