if(BUILD_MONGO_DB_PLUGIN)

  find_package(libmongoc-1.0 1.8)

  if (libmongoc-1.0_FOUND)

      find_package(libbson-1.0 REQUIRED)
      message(STATUS "Found bson headers: ${BSON_INCLUDE_DIRS}")

      find_package(libbsoncxx-static REQUIRED)
      find_package(libmongocxx-static REQUIRED)
      find_package(libmongoc-static-1.0 REQUIRED)
      find_package(libbson-static-1.0 REQUIRED)

  else()
      message(FATAL_ERROR "Could NOT find mongo-c-driver. Disable mongo support or ensure mongo-c-driver and mongo-cxx-driver is built and installed")
      return()
  endif()

  # This needs to be after the else/return in the situation that libmongoc isn't found and we need to avoid building mongo :: 'bsoncxx/builder/basic/kvp.hpp' file not found
  file(GLOB HEADERS "include/eosio/mongo_db_plugin/*.hpp")
  add_library( mongo_db_plugin
               mongo_db_plugin.cpp
               ${HEADERS} )

  target_include_directories(mongo_db_plugin
          PRIVATE ${LIBMONGOCXX_STATIC_INCLUDE_DIRS} ${LIBBSONCXX_STATIC_INCLUDE_DIRS} ${BSON_INCLUDE_DIRS}
          PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
          )

  target_compile_definitions(mongo_db_plugin
    PRIVATE ${LIBMONGOCXX_STATIC_DEFINITIONS} ${LIBBSONCXX_STATIC_DEFINITIONS}
    )

  # We can't just use *_STATIC_LIBRARIES variables to link against because the static
  # variants of these may try to static link against libraries we don't want (like a system
  # libc/c++). But we need to know if mongo c driver was built with ICU, SASL2, or snappy support
  # so that we can continue to link to those. This certainly is a bit on the fragile side but
  # try to parse what is included in MONGOC_STATIC_LIBRARIES to see what we should link to
  foreach(MONGO_S_LIB ${MONGOC_STATIC_LIBRARIES})
    string(REGEX MATCH "libsasl2\\${CMAKE_SHARED_LIBRARY_SUFFIX}$" REGOUT ${MONGO_S_LIB})
    if(REGOUT)
      set(LINK_SASL "sasl2")
    endif()

    string(REGEX MATCH "libicuuc\\${CMAKE_SHARED_LIBRARY_SUFFIX}$" REGOUT ${MONGO_S_LIB})
    if(REGOUT)
      set(LINK_ICU "icuuc")
    endif()

    string(REGEX MATCH "libsnappy\\${CMAKE_SHARED_LIBRARY_SUFFIX}$" REGOUT ${MONGO_S_LIB})
    if(REGOUT)
      set(LINK_SNAPPY "snappy")
    endif()
  endforeach()

  target_link_libraries(mongo_db_plugin
          PUBLIC chain_plugin eosio_chain appbase
          ${LIBMONGOCXX_STATIC_LIBRARY_PATH} ${LIBBSONCXX_STATIC_LIBRARY_PATH}
          ${MONGOC_STATIC_LIBRARY} ${BSON_STATIC_LIBRARY}
          resolv ${LINK_SASL} ${LINK_ICU} ${LINK_SNAPPY}
          )

else()
  message("mongo_db_plugin not selected and will be omitted.")
endif()
