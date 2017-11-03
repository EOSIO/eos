file(GLOB HEADERS "include/eos/db_plugin/*.hpp")
add_library( db_plugin
             db_plugin.cpp
             ${HEADERS} )

find_package(libmongoc-1.0 1.8)
find_package(libmongocxx)

if (libmongocxx_FOUND)
    message("--   mongoc found version \"${MONGOC_VERSION}\"")
    message("--   mongoc include path \"${MONGOC_INCLUDE_DIRS}\"")
    message("--   mongoc libraries \"${MONGOC_LIBRARIES}\"")
    message("--   mongocxx found version \"${MONGOCXX_VERSION}\"")
    message("--   mongocxx include path \"${MONGOCXX_INCLUDE_DIRS}\"")
    message("--   mongoc libraries \"${MONGOCXX_LIBRARIES}\"")
    link_directories(
            ${MONGOCXX_LIBRARY_DIRS}
            ${BSONCXX_LIBRARY_DIRS}
    )
    add_definitions(-DMONGODB)
else()
    message("Could NOT find MongoDB. db_plugin with MongoDB support will not be included.")
endif()

target_include_directories(db_plugin
        PRIVATE ${MONGOCXX_INCLUDE_DIRS}
        PRIVATE ${BSONCXX_INCLUDE_DIRS}
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
        )
target_link_libraries(db_plugin
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
