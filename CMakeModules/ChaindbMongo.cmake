set(CHAINDB_DEFS "")
set(CHAINDB_INCS "")
set(CHAINDB_LIBS "")
set(CHAINDB_SRCS "")

if (BUILD_CYBERWAY_CHAINDB_MONGO)
    message("chaindb with MongoDB support is selected and will be builded.")

    find_package(libmongoc-1.0 1.8)

    if (libmongoc-1.0_FOUND)
        find_package(libbsoncxx-static REQUIRED)
        message(STATUS "Found bsoncxx headers for chaindb: ${LIBBSONCXX_STATIC_INCLUDE_DIRS}")

        if ((LIBBSONCXX_STATIC_VERSION_MAJOR LESS 3) OR ((LIBBSONCXX_STATIC_VERSION_MAJOR EQUAL 3) AND (LIBBSONCXX_STATIC_VERSION_MINOR LESS 2)))
            find_library(CHAINDB_LIBBSONCXX ${LIBBSONCXX_STATIC_LIBRARIES}
                PATHS ${LIBBSONCXX_STATIC_LIBRARY_DIRS} NO_DEFAULT_PATH)
        else()
            set(CHAINDB_LIBBSONCXX ${LIBBSONCXX_STATIC_LIBRARIES})
        endif()
        message(STATUS "Found bsoncxx library for chaindb: ${CHAINDB_LIBBSONCXX}")

        find_package(libmongocxx-static REQUIRED)
        message(STATUS "Found mongocxx headers for chaindb: ${LIBMONGOCXX_STATIC_INCLUDE_DIRS}")

        # mongo-cxx-driver 3.2 release altered LIBBSONCXX_LIBRARIES semantics. Instead of library names,
        #  it now hold library paths.
        if ((LIBMONGOCXX_STATIC_VERSION_MAJOR LESS 3) OR ((LIBMONGOCXX_STATIC_VERSION_MAJOR EQUAL 3) AND (LIBMONGOCXX_STATIC_VERSION_MINOR LESS 2)))
            find_library(CHAINDB_LIBMONGOCXX ${LIBMONGOCXX_STATIC_LIBRARIES}
                PATHS ${LIBMONGOCXX_STATIC_LIBRARY_DIRS} NO_DEFAULT_PATH)
        else()
            set(CHAINDB_LIBMONGOCXX ${LIBMONGOCXX_STATIC_LIBRARIES})
        endif()
        message(STATUS "Found mongocxx library for chaindb: ${CHAINDB_LIBMONGOCXX}")

        set(CHAINDB_DEFS ${CHAINDB_DEFS} CYBERWAY_CHAINDB=1 ${LIBMONGOCXX_STATIC_DEFINITIONS} ${LIBBSONCXX_STATIC_DEFINITIONS})
        set(CHAINDB_INCS ${CHAINDB_INCS} ${BSON_INCLUDE_DIRS} ${LIBMONGOCXX_STATIC_INCLUDE_DIRS} ${LIBBSONCXX_STATIC_INCLUDE_DIRS})
        set(CHAINDB_LIBS ${CHAINDB_LIBS} ${CHAINDB_LIBMONGOCXX} ${CHAINDB_LIBBSONCXX} ${BSON_LIBRARIES})
        set(CHAINDB_SRCS ${CHAINDB_SRCS} chaindb/mongo_driver.cpp chaindb/mongo_driver_utils.cpp chaindb/mongo_bigint_converter.cpp)

    else()
        message("Could NOT find MongoDB. chaindb with MongoDB support will not be included.")
        set(CHAINDB_SRCS ${CHAINDB_SRCS} chaindb/mongo_fail_driver.cpp)
    endif()
else()
    message("chaindb with MongoDB support not selected and will be omitted.")
    set(CHAINDB_SRCS ${CHAINDB_SRCS} chaindb/mongo_fail_driver.cpp)
endif()
