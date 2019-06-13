cmake_minimum_required(VERSION 3.5)
project(Version)

# Define the default version metadata in case `git` cannot be found.
set(_VERSION_MAJOR_  "unknown")
set(_VERSION_MINOR_  "")
set(_VERSION_PATCH_  "")
set(_VERSION_SUFFIX_ "")
set(_VERSION_HASH_   "")
set(_VERSION_DIRTY_  "")

# Construct the library.
add_library(
  version
  "${CMAKE_CURRENT_SOURCE_DIR}/src/version.cpp"
  "${CMAKE_CURRENT_BINARY_DIR}/src/version_impl.cpp")

# Make dependencies visible.
target_include_directories(
  version
  PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}/include/"
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/" )

# Create a custom target to be run upon every build.
find_package(Git)
if(EXISTS ${CMAKE_SOURCE_DIR}/.git AND ${GIT_FOUND})
   add_custom_target(
      evaluate_every_build ALL
      COMMAND ${CMAKE_COMMAND} -DGIT_EXEC=${GIT_EXECUTABLE}
                               -DLIB_BIN_DIR=${CMAKE_CURRENT_BINARY_DIR}
                               -DLIB_CUR_DIR=${CMAKE_CURRENT_SOURCE_DIR}
                               -DROOT_DIR=${CMAKE_SOURCE_DIR}
                               -DV_MAJOR=${VERSION_MAJOR}
                               -DV_MINOR=${VERSION_MINOR}
                               -DV_PATCH=${VERSION_PATCH}
                               -DV_SUFFIX=${VERSION_SUFFIX}
                               -P ${CMAKE_SOURCE_DIR}/CMakeModules/VersionUtils.cmake
      COMMENT "Updating version metadata.." VERBATIM )
else()
   # Modify and substitute the `.cpp.in` file for a `.cpp` in the build directory.
   configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/src/version_impl.cpp.in
      ${CMAKE_CURRENT_BINARY_DIR}/src/version_impl.cpp
      @ONLY )
endif()

# ...
set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/src/version_impl.cpp PROPERTY GENERATED 1)

# ...
add_dependencies(version evaluate_every_build)

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
