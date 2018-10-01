find_package(libmongoc-1.0 1.8)

if (libmongoc-1.0_FOUND)
    find_package(libbsoncxx-static REQUIRED)
    message(STATUS "Found bsoncxx headers: ${LIBBSONCXX_STATIC_INCLUDE_DIRS}")
    if((LIBBSONCXX_STATIC_VERSION_MAJOR LESS 3) OR ((LIBBSONCXX_STATIC_VERSION_MAJOR EQUAL 3) AND (LIBBSONCXX_STATIC_VERSION_MINOR LESS 2)))
        find_library(CHAINDB_LIBBSONCXX ${LIBBSONCXX_STATIC_LIBRARIES}
                PATHS ${LIBBSONCXX_STATIC_LIBRARY_DIRS} NO_DEFAULT_PATH)
    else()
        set(CHAINDB_LIBBSONCXX ${LIBBSONCXX_STATIC_LIBRARIES})
    endif()

    message(STATUS "Found bsoncxx library: ${CHAINDB_LIBBSONCXX}")

    find_package(libmongocxx-static REQUIRED)
    message(STATUS "Found mongocxx headers: ${LIBMONGOCXX_STATIC_INCLUDE_DIRS}")

    # mongo-cxx-driver 3.2 release altered LIBBSONCXX_LIBRARIES semantics. Instead of library names,
    #  it now hold library paths.
    if((LIBMONGOCXX_STATIC_VERSION_MAJOR LESS 3) OR ((LIBMONGOCXX_STATIC_VERSION_MAJOR EQUAL 3) AND (LIBMONGOCXX_STATIC_VERSION_MINOR LESS 2)))
        find_library(CHAINDB_LIBMONGOCXX ${LIBMONGOCXX_STATIC_LIBRARIES}
                PATHS ${LIBMONGOCXX_STATIC_LIBRARY_DIRS} NO_DEFAULT_PATH)
    else()
        set(CHAINDB_LIBMONGOCXX ${LIBMONGOCXX_STATIC_LIBRARIES})
    endif()

    set(CHAINDB_LIBBSON ${BSON_LIBRARIES})

    message(STATUS "Found mongocxx library: ${CHAINDB_LIBMONGOCXX}")
else()
    message("Could NOT find MongoDB. mongo_db_plugin with MongoDB support will not be included.")
endif()

set(CHAINDB_TEST_NAME chaindb_test)
add_executable(${CHAINDB_TEST_NAME} main.cpp multi_index.hpp chaindb.h chaindb.cpp chaindb_control.hpp)

target_include_directories(
    ${CHAINDB_TEST_NAME}
    PRIVATE ${BSON_INCLUDE_DIRS} ${LIBMONGOCXX_STATIC_INCLUDE_DIRS} ${LIBBSONCXX_STATIC_INCLUDE_DIRS}
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${Boost_INCLUDE_DIR})

target_compile_definitions(
    ${CHAINDB_TEST_NAME}
    PRIVATE ${LIBMONGOCXX_STATIC_DEFINITIONS} ${LIBBSONCXX_STATIC_DEFINITIONS})

target_link_libraries(
    ${CHAINDB_TEST_NAME}
    PRIVATE ${PLATFORM_SPECIFIC_LIBS} ${Boost_LIBRARIES} fc eosio_chain
    ${CHAINDB_LIBMONGOCXX} ${CHAINDB_LIBBSONCXX} ${CHAINDB_LIBBSON})

install(TARGETS
   ${CHAINDB_TEST_NAME}

   RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
   LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
   ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)
