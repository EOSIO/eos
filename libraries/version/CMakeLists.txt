cmake_minimum_required(VERSION 3.5)
project(Version)

# Define the version metadata by default, in case `git` cannot be found.
set(_VERSION_MAJOR_  "unknown")
set(_VERSION_MINOR_  "")
set(_VERSION_PATCH_  "")
set(_VERSION_SUFFIX_ "")
set(_VERSION_HASH_   "")
set(_VERSION_DIRTY_  "")

# Construct the library target.
add_library(
   version
   "${CMAKE_CURRENT_SOURCE_DIR}/src/version.cpp"
   "${CMAKE_CURRENT_BINARY_DIR}/src/version_impl.cpp")

# Make dependencies visible to the given target library to be constructed.
target_include_directories(
   version
   PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}/include/"
   PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/" )

# Create a custom target to update the version metadata upon every build.
find_package(Git)
if(EXISTS ${CMAKE_SOURCE_DIR}/.git AND ${GIT_FOUND})
   add_custom_target(
      evaluate_every_build ALL
      COMMAND ${CMAKE_COMMAND} -DGIT_EXEC=${GIT_EXECUTABLE}
                               -DCUR_BIN_DIR=${CMAKE_CURRENT_BINARY_DIR}
                               -DCUR_SRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}
                               -DSRC_DIR=${CMAKE_SOURCE_DIR}
                               -DV_MAJOR=${VERSION_MAJOR}
                               -DV_MINOR=${VERSION_MINOR}
                               -DV_PATCH=${VERSION_PATCH}
                               -DV_SUFFIX=${VERSION_SUFFIX}
                               -P ${CMAKE_SOURCE_DIR}/CMakeModules/VersionUtils.cmake
      BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/src/version_impl.cpp
      COMMENT "Updating version metadata..." VERBATIM )
                             
      # Create a dependency for the given library target.
      add_dependencies(version evaluate_every_build)
else()
   # Modify and substitute the `.cpp.in` file for a `.cpp` in the build directory.
   configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/src/version_impl.cpp.in
      ${CMAKE_CURRENT_BINARY_DIR}/src/version_impl.cpp
      @ONLY )
endif()
