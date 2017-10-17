file(GLOB HEADERS "include/eosio/blockchain/*.hpp")

add_library( eosio_blockchain name.cpp generic_table.cpp db.cpp controller.cpp )
target_link_libraries( eosio_blockchain fc chainbase eos_types )
target_include_directories( eosio_blockchain PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/inlcude" "${CMAKE_CURRENT_SOURCE_DIR}/include"
                                                   "${CMAKE_CURRENT_SOURCE_DIR}/../wasm-jit/Include" )

add_executable( blockchain main.cpp )
target_link_libraries( blockchain fc eos_types eosio_blockchain)
