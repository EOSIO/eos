cmake_minimum_required( VERSION 3.5 )

project(contracts)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_subdirectory(eosio.bios)
add_subdirectory(eosio.msig)
add_subdirectory(eosio.system)
add_subdirectory(eosio.token)
add_subdirectory(eosio.wrap)
