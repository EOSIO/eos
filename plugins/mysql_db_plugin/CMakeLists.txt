if (BUILD_MYSQL_DB_PLUGIN)
    file(GLOB HEADERS "include/eosio/mysql_db_plugin/*.hpp")
    add_library(mysql_db_plugin
            mysql_db_plugin.cpp odb/eos-odb.cxx
            action_handler.cpp action_handler.hpp
            try_handle.hpp try_handle.cpp
            ${HEADERS})

    find_package(Odb)

    target_include_directories(mysql_db_plugin PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/odb
            ${ODB_INCLUDE_DIRS})
    target_link_libraries(mysql_db_plugin chain_plugin appbase fc ${ODB_LIBRARY})
else ()
    message("mysql_db_plugin not selected and will be omitted.")
endif ()
