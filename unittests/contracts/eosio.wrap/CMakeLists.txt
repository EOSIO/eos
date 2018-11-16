set( CMAKE_CXX_STANDARD 17 )

include_directories( ${MYPROJECT_DIR}./eosio.wrap/ )

add_contract( eosio.wrap eosio.wrap eosio.wrap.cpp )
