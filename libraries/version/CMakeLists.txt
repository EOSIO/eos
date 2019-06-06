cmake_minimum_required(
  VERSION 2.8.12 ) # What should the version actually be?

project(
  Version )

# Found in directory `eos/CMakeModules/`.
include(
  VersionUtils )

# Generate the most up-to-date metadata of the repository.
GENERATE_VERSION_METADATA()

# Construct the library.
add_library(
   version
   "${CMAKE_CURRENT_SOURCE_DIR}/src/version.cpp" )

# Make dependencies visible.
target_include_directories(
   version
   PUBLIC
   "${CMAKE_CURRENT_SOURCE_DIR}/include/" )

# Install the library in the appropriate places.
install(
   TARGETS
   version
   RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}   #  Is it needed as a RUNTIME? It' just a library?
   ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}   ## Is it needed as both a static and dynamic lib?
   LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} ) ## ...

# Install the header file in the appropriate place.
install(
   DIRECTORY
   ${CMAKE_CURRENT_SOURCE_DIR}/include/eosio/version
   DESTINATION
   ${CMAKE_INSTALL_FULL_INCLUDEDIR}/eosio/
   FILES_MATCHING PATTERN "*.hpp" )

# Modify and substitute the `.cpp.in` file for a `.cpp` in the build directory.
configure_file(
   ${CMAKE_CURRENT_SOURCE_DIR}/src/version_impl.cpp.in
   ${CMAKE_CURRENT_BINARY_DIR}/src/version_impl.cpp
   @ONLY )
