file(GLOB_RECURSE HEADERS "include/*.hpp")

add_library( chain_kv INTERFACE )

target_link_libraries( chain_kv
                       INTERFACE fc rocksdb softfloat
                     )

target_include_directories( chain_kv
                            INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include/eosio"
                          )

add_subdirectory(unit_tests)
