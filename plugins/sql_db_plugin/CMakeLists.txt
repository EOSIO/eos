find_package(Soci)
if(NOT SOCI_FOUND)
    message(STATUS "Database SQL plugin: disabled")
    return()
endif()

message(STATUS "Database SQL plugin: enabled")

include_directories(${CMAKE_CURRENT_SOURCE_DIR} include db)

add_library(sql_db_plugin
    db/database.cpp
    db/accounts_table.cpp
    db/transactions_table.cpp
    db/blocks_table.cpp
    db/actions_table.cpp
    sql_db_plugin.cpp
    irreversible_block_storage.cpp
    block_storage.cpp
    )

target_link_libraries(sql_db_plugin
    chain_plugin
    ${SOCI_LIBRARY}
    )

add_subdirectory(test)

