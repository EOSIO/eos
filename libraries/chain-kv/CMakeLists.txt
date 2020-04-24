file(GLOB_RECURSE HEADERS "include/*.hpp")

add_library( chain-kv INTERFACE )

target_link_libraries( chain-kv 
                       INTERFACE fc rocksdb gflags
                     )

target_include_directories( chain-kv
                            INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../rocksdb/include"
                          )

add_subdirectory(unit_tests)
