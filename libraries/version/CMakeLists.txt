cmake_minimum_required(
  VERSION 2.8.12 ) # What should the version actually be?

project(
  Version )

add_library(
   version
   "./src/version.cpp"
   "./include/eosio/version/version.hpp" )

target_include_directories(
   version
   PUBLIC
   "${CMAKE_CURRENT_SOURCE_DIR}/include/" )

install(
   TARGETS
   version
   RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}   #  Is it needed as a RUNTIME? It' just a library?
   ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}   ## Is it needed as both a static and dynamic lib?
   LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} ) ## ...

install(
   DIRECTORY
   ${CMAKE_CURRENT_SOURCE_DIR}/include/eosio/version
   DESTINATION
   ${CMAKE_INSTALL_FULL_INCLUDEDIR}/eosio/version
   "./include/eosio/version/version.hpp" )
