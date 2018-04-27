add_executable( sql_db_plugin_test test.cpp )
target_link_libraries( sql_db_plugin_test
    sql_db_plugin
    ${Boost_LIBRARIES}
    )
