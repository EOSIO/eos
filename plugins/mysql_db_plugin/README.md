# eosio_mysql_plugin

EOSIO plugin to register blockchain data into an MySQL database supported by libzdb ( https://www.tildeslash.com/libzdb/ )

**You need to compile EOSIO from source: additional plugins are linked statically**



add the following param on eosio_build.sh:
```
-DBUILD_MYSQL_DB_PLUGIN=true
```
add the following phrase on programs/nodeos/CMakeLists.txt:
```
if(BUILD_MYSQL_DB_PLUGIN)
  target_link_libraries( ${NODE_EXECUTABLE_NAME} PRIVATE -Wl,${whole_archive_flag} mysql_db_plugin -Wl,${no_whole_archive_flag} )
endif()
```

add the following phrase on plugins/CMakeLists.txt:
```
add_subdirectory(mysql_db_plugin)
```

compile and run.
```
$ nodeos --help

....
Config Options for eosio::mysql_db_plugin:
    --mysqldb-wipe = true               if true, wipe all tables from database
    --mysqldb-queue-size  arg (=256)    The queue size between nodeos and MySQL 
                                        DB plugin thread.
    --mysqldb-uri = arg                 MySQL DB URI connection string.
                                        If not specified then plugin is disabled. 
                                        Database name has to be specified.
                                        e.g. mysql://<host-ip>:3306/<db name>?user=<user>&password=<password>
....
```
