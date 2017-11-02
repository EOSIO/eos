file(GLOB HEADERS "include/eos/db_plugin/*.hpp")
add_library( db_plugin
             db_plugin.cpp
             ${HEADERS} )

find_package(libmongoc-1.0 1.8)

if (libmongoc-1.0_FOUND)
    message("--   mongoc found version \"${MONGOC_VERSION}\"")
    message("--   mongoc include path \"${MONGOC_INCLUDE_DIRS}\"")
    message("--   mongoc libraries \"${MONGOC_LIBRARIES}\"")

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
    add_definitions(-DMONGODB)
else()
    message("Could NOT find MongoDB. db_plugin with MongoDB support will not be included.")
    # sudo apt-get install pkg-config libssl-dev libsasl2-dev
    # wget https://github.com/mongodb/mongo-c-driver/releases/download/1.8.0/mongo-c-driver-1.8.0.tar.gz
    # tar xzf mongo-c-driver-1.8.0.tar.gz
    # cd mongo-c-driver-1.8.0
    # ./configure --disable-automatic-init-and-cleanup --enable-static
    # make
    # sudo make install
    #
    # git clone https://github.com/mongodb/mongo-cxx-driver.git --branch releases/stable --depth 1
    # cd mongo-cxx-driver/build
    # cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
    # sudo make EP_mnmlstc_core
    # make
    # sudo make install
    #
    # sudo apt-get install mongodb
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
