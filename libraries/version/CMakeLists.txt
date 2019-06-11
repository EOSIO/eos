cmake_minimum_required(
  VERSION 3.5 )

project(
   Version )

# Found in directory `eos/CMakeModules/`.
include(
  VersionUtils )

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

add_custom_target( on_every_build ALL
   COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/CMakeModules/VersionUtils.cmake
   COMMENT "generating........................" VERBATIM)

# # Generate the most up-to-date version metadata of the repository on every build.
# add_custom_target(
#    on_every_build
#    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/CMakeModules/VersionUtils.cmake )

# # ...
# add_dependencies(
#    version
#    on_every_build )

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
