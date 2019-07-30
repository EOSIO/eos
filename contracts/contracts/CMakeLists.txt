cmake_minimum_required( VERSION 3.5 )

project(contracts)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_subdirectory(rem.bios)
add_subdirectory(rem.msig)
add_subdirectory(rem.system)
add_subdirectory(rem.token)
add_subdirectory(rem.wrap)
