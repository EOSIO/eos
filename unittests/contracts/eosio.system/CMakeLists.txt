set( CMAKE_CXX_STANDARD 17 )

include_directories( ${MYPROJECT_DIR}./eosio.system/ )
include_directories( ${MYPROJECT_DIR}../eosio.token/ )

add_contract( eosio.system eosio.system delegate_bandwidth.cpp eosio.system.cpp exchange_state.cpp producer_pay.cpp voting.cpp )
