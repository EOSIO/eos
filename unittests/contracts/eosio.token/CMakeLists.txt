set( CMAKE_CXX_STANDARD 17 )

include_directories( ${MYPROJECT_DIR}./eosio.token/ )

add_contract( eosio.token eosio.token eosio.token.cpp )
