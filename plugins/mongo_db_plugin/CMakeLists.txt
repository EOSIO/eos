if(BUILD_MONGO_DB_PLUGIN)

  find_package(libmongoc-1.0 1.8)

  if (libmongoc-1.0_FOUND)

      find_package(libbsoncxx-static REQUIRED)
      find_library(EOS_LIBBSONCXX "libbsoncxx-static${CMAKE_STATIC_LIBRARY_SUFFIX}" PATHS ${LIBBSONCXX_STATIC_LIBRARY_DIRS})

      find_package(libmongocxx-static REQUIRED)
      find_library(EOS_LIBMONGOCXX "libmongocxx-static${CMAKE_STATIC_LIBRARY_SUFFIX}" PATHS ${LIBMONGOCXX_STATIC_LIBRARY_DIRS})

      find_package(libmongoc-static-1.0 REQUIRED)
      find_library(EOS_LIBMONGOC "libmongoc-static-1.0${CMAKE_STATIC_LIBRARY_SUFFIX}" PATHS ${LIBMONGOC_STATIC_LIBRARY_DIRS})

      find_package(libbson-static-1.0 REQUIRED)
      find_library(EOS_LIBBSONC "libbson-static-1.0${CMAKE_STATIC_LIBRARY_SUFFIX}" PATHS ${LIBBSON_STATIC_LIBRARY_DIRS})

      if(NOT EOS_LIBBSONCXX OR NOT EOS_LIBMONGOCXX OR NOT EOS_LIBMONGOC OR NOT EOS_LIBBSONC)
          message(FATAL_ERROR "Could not find one or more mongo driver static libraries")
      endif()

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
          ${EOS_LIBMONGOCXX} ${EOS_LIBBSONCXX} ${EOS_LIBMONGOC} ${EOS_LIBBSONC} resolv
          )
               
else()
  message("mongo_db_plugin not selected and will be omitted.")
endif()
