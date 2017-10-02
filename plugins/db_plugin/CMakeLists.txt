file(GLOB HEADERS "include/eos/db_plugin/*.hpp")
add_library( db_plugin
             db_plugin.cpp
             ${HEADERS} )

find_package(libmongoc-1.0 1.8 REQUIRED)

message ("--   mongoc found version \"${MONGOC_VERSION}\"")
message ("--   mongoc include path \"${MONGOC_INCLUDE_DIRS}\"")
message ("--   mongoc libraries \"${MONGOC_LIBRARIES}\"")

find_package(PkgConfig QUIET)

# NOTE: For this to work, the PKG_CONFIG_PATH variable (man pkg-config) must be set to point to the
# 'lib/pkgconfig' subdirectory of the directory used as the argument to CMAKE_INSTALL_PREFIX when
# building libmongocxx and libbsoncxx.
pkg_search_module(BSONCXX REQUIRED libbsoncxx)
pkg_search_module(MONGOCXX REQUIRED libmongocxx)

link_directories(
  ${MONGOCXX_LIBRARY_DIRS}
  ${BSONCXX_LIBRARY_DIRS}
)

target_include_directories( db_plugin
        PRIVATE ${MONGOCXX_INCLUDE_DIRS}
        PRIVATE ${BSONCXX_INCLUDE_DIRS}
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries( db_plugin
        PUBLIC chain_plugin eos_chain appbase
        PRIVATE ${MONGOCXX_LIBRARIES} ${BSONCXX_LIBRARIES}
)



install( TARGETS
   db_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/eos/db_plugin" )
