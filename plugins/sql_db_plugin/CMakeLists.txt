add_subdirectory(externals)

include_directories(include)
add_library(sql_db_plugin
    sql_db_plugin.cpp
    accounts_table.cpp
    transactions_table.cpp
    blocks_table.cpp
    actions_table.cpp
    )
target_link_libraries(sql_db_plugin chain_plugin)

add_subdirectory(test)

