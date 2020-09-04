add_executable( test_kv_rocksdb test_kv_rocksdb.cpp )
target_link_libraries( test_kv_rocksdb eosio_chain fc chainbase )
target_include_directories( test_kv_rocksdb PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../include")
add_test(NAME test_kv_rocksdb COMMAND libraries/chain/backing_store/tests/test_kv_rocksdb WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
