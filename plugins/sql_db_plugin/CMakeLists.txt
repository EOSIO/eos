find_package(Soci REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} include)

add_library(sql_db_plugin
    sql_db_plugin.cpp
    database.cpp
    accounts_table.cpp
    transactions_table.cpp
    blocks_table.cpp
    actions_table.cpp
    )

target_link_libraries(sql_db_plugin
    chain_plugin
    ${SOCI_LIBRARY}
    )

add_subdirectory(test)

