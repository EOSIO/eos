file(GLOB HEADERS "../include/eosio/blockchain/*.hpp")

add_executable( test_key_recovery_service test_key_recovery_service.cpp)
#target_include_directories( test_key_recovery_service PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../include" )
target_link_libraries( test_key_recovery_service fc eosio_blockchain )

