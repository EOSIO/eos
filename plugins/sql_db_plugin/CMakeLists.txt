include_directories(include)
add_library(sql_db_plugin sql_db_plugin.cpp)
target_link_libraries(sql_db_plugin chain_plugin)

