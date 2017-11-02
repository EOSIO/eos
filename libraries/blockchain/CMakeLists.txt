file(GLOB HEADERS "include/eosio/blockchain/*.hpp" "include/eosio/blockchain/internal/*.hpp")

add_library( eosio_blockchain 
             wast_to_wasm.cpp
             wasm.cpp
             scheduler.cpp 
             transaction_scheduler.cpp
             transaction.cpp 
             transaction_cache.cpp 
             name.cpp 
             public_key.cpp 
             block.cpp 
             merkle.cpp 
             generic_table.cpp 
             database.cpp 
             fork_database.cpp 
             block_log.cpp 
             controller.cpp )

target_link_libraries( eosio_blockchain fc chainbase eos_types  Logging IR WAST WASM Runtime )
target_include_directories( eosio_blockchain PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/inlcude" "${CMAKE_CURRENT_SOURCE_DIR}/include"
                                                   "${CMAKE_CURRENT_SOURCE_DIR}/../wasm-jit/Include" )

add_executable( blockchain main.cpp)
target_link_libraries( blockchain fc eos_types eosio_blockchain)
target_include_directories( blockchain PUBLIC ${CMAKE_BINARY_DIR}/contracts )

add_subdirectory( test )
