set( CMAKE_CXX_STANDARD 17 )

include_directories( ${MYPROJECT_DIR}../ )

add_contract_no_abi( eosio.token eosio.token eosio.token.cpp )

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/eosio.token.abi ${CMAKE_CURRENT_BINARY_DIR}/eosio.token.abi COPYONLY )

