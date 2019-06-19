if(BUILD_MONGO_DB_PLUGIN)

  find_package(libmongoc-1.0 1.8)

  if (libmongoc-1.0_FOUND)

      # EOS has no direct dependencies on libmongoc but its shared libraries
      # will need to be present at runtime for the C++ libraries we use:
      # libbsoncxx & libmongocxx (both from github.com/mongodb/mongo-cxx-driver)

      # The *.cmake package files provided by mongo-cxx-driver don't give us the
      # absolute path to the libraries, which is needed whenever they are not
      # installed in system-known locations.  CMake requires the absolute paths
      # in target_link_libraries() since we are builiding an archive and the
      # link step for all executables using this archive must include the
      # mongo-cxx-driver libraries libmongocxx and libbsoncxx.

      find_package(libbsoncxx-static REQUIRED)
      message(STATUS "Found bsoncxx headers: ${LIBBSONCXX_STATIC_INCLUDE_DIRS}")

      # mongo-cxx-driver 3.2 release altered LIBBSONCXX_LIBRARIES semantics. Instead of library names,
      #  it now hold library paths.
      if((LIBBSONCXX_STATIC_VERSION_MAJOR LESS 3) OR ((LIBBSONCXX_STATIC_VERSION_MAJOR EQUAL 3) AND (LIBBSONCXX_STATIC_VERSION_MINOR LESS 2)))
        find_library(EOS_LIBBSONCXX ${LIBBSONCXX_STATIC_LIBRARIES}
          PATHS ${LIBBSONCXX_STATIC_LIBRARY_DIRS} NO_DEFAULT_PATH)
      else()
        set(EOS_LIBBSONCXX ${LIBBSONCXX_STATIC_LIBRARIES})
      endif()

      message(STATUS "Found bsoncxx library: ${EOS_LIBBSONCXX}")

      find_package(libmongocxx-static REQUIRED)
      message(STATUS "Found mongocxx headers: ${LIBMONGOCXX_STATIC_INCLUDE_DIRS}")

      # mongo-cxx-driver 3.2 release altered LIBBSONCXX_LIBRARIES semantics. Instead of library names,
      #  it now hold library paths.
      if((LIBMONGOCXX_STATIC_VERSION_MAJOR LESS 3) OR ((LIBMONGOCXX_STATIC_VERSION_MAJOR EQUAL 3) AND (LIBMONGOCXX_STATIC_VERSION_MINOR LESS 2)))
        find_library(EOS_LIBMONGOCXX ${LIBMONGOCXX_STATIC_LIBRARIES}
          PATHS ${LIBMONGOCXX_STATIC_LIBRARY_DIRS} NO_DEFAULT_PATH)
      else()
        set(EOS_LIBMONGOCXX ${LIBMONGOCXX_STATIC_LIBRARIES})
      endif()

      message(STATUS "Found mongocxx library: ${EOS_LIBMONGOCXX}")
  else()
      message("Could NOT find MongoDB. mongo_db_plugin with MongoDB support will not be included.")
      return()
  endif()

  # This needs to be after the else/return in the situation that libmongoc isn't found and we need to avoid building mongo :: 'bsoncxx/builder/basic/kvp.hpp' file not found
  file(GLOB HEADERS "include/eosio/mongo_db_plugin/*.hpp")
  add_library( mongo_db_plugin
               mongo_db_plugin.cpp
               ${HEADERS} )

  target_include_directories(mongo_db_plugin
          PRIVATE ${LIBMONGOCXX_STATIC_INCLUDE_DIRS} ${LIBBSONCXX_STATIC_INCLUDE_DIRS}
          PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
          )

  target_compile_definitions(mongo_db_plugin
    PRIVATE ${LIBMONGOCXX_STATIC_DEFINITIONS} ${LIBBSONCXX_STATIC_DEFINITIONS}
    )

  target_link_libraries(mongo_db_plugin
          PUBLIC chain_plugin eosio_chain appbase
          ${EOS_LIBMONGOCXX} ${EOS_LIBBSONCXX}
          )
               
else()
  message("mongo_db_plugin not selected and will be omitted.")
endif()
