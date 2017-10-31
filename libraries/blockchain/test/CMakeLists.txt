file(GLOB HEADERS "../include/eosio/blockchain/*.hpp")

add_executable( test_key_recovery_service test_key_recovery_service.cpp)
target_link_libraries( test_key_recovery_service fc eosio_blockchain )

add_executable( test_incremental_merkle test_incremental_merkle.cpp)
target_link_libraries( test_incremental_merkle fc eosio_blockchain )
