set( CMAKE_CXX_STANDARD 17 )

include_directories( ${MYPROJECT_DIR}./eosio.msig/ )

add_contract( eosio.msig eosio.msig eosio.msig.cpp )
