set( CMAKE_CXX_STANDARD 17 )

include_directories( ${MYPROJECT_DIR}./eosio.bios/ )

add_contract( eosio.bios eosio.bios eosio.bios.cpp )
